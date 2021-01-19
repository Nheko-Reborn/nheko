#include "notifications/Manager.h"

#include <QDebug>
#include <QImage>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

NotificationsManager::NotificationsManager(QObject *parent)
  : QObject(parent)
  , dbus("org.freedesktop.Notifications",
         "/org/freedesktop/Notifications",
         "org.freedesktop.Notifications",
         QDBusConnection::sessionBus(),
         this)
{
        qDBusRegisterMetaType<QImage>();

        QDBusConnection::sessionBus().connect("org.freedesktop.Notifications",
                                              "/org/freedesktop/Notifications",
                                              "org.freedesktop.Notifications",
                                              "ActionInvoked",
                                              this,
                                              SLOT(actionInvoked(uint, QString)));
        QDBusConnection::sessionBus().connect("org.freedesktop.Notifications",
                                              "/org/freedesktop/Notifications",
                                              "org.freedesktop.Notifications",
                                              "NotificationClosed",
                                              this,
                                              SLOT(notificationClosed(uint, uint)));
        QDBusConnection::sessionBus().connect("org.freedesktop.Notifications",
                                              "/org/freedesktop/Notifications",
                                              "org.freedesktop.Notifications",
                                              "NotificationReplied",
                                              this,
                                              SLOT(notificationReplied(uint, QString)));
}

/**
 * This function is based on code from
 * https://github.com/rohieb/StratumsphereTrayIcon
 * Copyright (C) 2012 Roland Hieber <rohieb@rohieb.name>
 * Licensed under the GNU General Public License, version 3
 */
void
NotificationsManager::postNotification(const QString &roomid,
                                       const QString &eventid,
                                       const QString &roomname,
                                       const QString &sender,
                                       const QString &text,
                                       const QImage &icon)
{
    Q_UNUSED(icon)

    QVariantMap hints;
    hints["image-data"] = sender + ": " + text;
    hints["sound-name"] = "message-new-instant";
    QList<QVariant> argumentList;
    argumentList << "nheko"; // app_name
    argumentList << (uint)0; // replace_id
    argumentList << "";      // app_icon
    argumentList << roomname; // summary
    argumentList << text;    // body
    // The list of actions has always the action name and then a localized version of that
    // action. Currently we just use an empty string for that.
    // TODO(Nico): Look into what to actually put there.
    argumentList << (QStringList("default") << ""
                                            << "inline-reply"
                                            << ""); // actions
    argumentList << hints;                          // hints
    argumentList << (int)-1;                        // timeout in ms

    static QDBusInterface notifyApp("org.freedesktop.Notifications",
                                    "/org/freedesktop/Notifications",
                                    "org.freedesktop.Notifications");
    auto call =
      notifyApp.callWithArgumentList(QDBus::AutoDetect, "Notify", argumentList);
    QDBusPendingCallWatcher watcher{QDBusPendingReply{call}};
    connect(&watcher, &QDBusPendingCallWatcher::finished, this, [&watcher, this, &roomid, &eventid]() {
        if (watcher.reply().type() == QDBusMessage::ErrorMessage)
                qDebug() << "D-Bus Error:" << watcher.reply().errorMessage();
        else
            notificationIds[watcher.reply().arguments().first().toUInt()] = roomEventId{roomid, eventid};
    });
}

uint
NotificationsManager::showNotification(const QString summary,
                                       const QString text,
                                       const QImage image)
{
    Q_UNUSED(summary)
    Q_UNUSED(text)
    Q_UNUSED(image)

    return 0;
}

void
NotificationsManager::closeNotification(uint id)
{
        QList<QVariant> argumentList;
        argumentList << (uint)id; // replace_id

        static QDBusInterface closeCall("org.freedesktop.Notifications",
                                        "/org/freedesktop/Notifications",
                                        "org.freedesktop.Notifications");
        QDBusMessage reply =
          closeCall.callWithArgumentList(QDBus::AutoDetect, "CloseNotification", argumentList);
        if (reply.type() == QDBusMessage::ErrorMessage) {
                qDebug() << "D-Bus Error:" << reply.errorMessage();
        }
}

void
NotificationsManager::removeNotification(const QString &roomId, const QString &eventId)
{
        roomEventId reId = {roomId, eventId};
        for (auto elem = notificationIds.begin(); elem != notificationIds.end(); ++elem) {
                if (elem.value().roomId != roomId)
                        continue;

                // close all notifications matching the eventId or having a lower
                // notificationId
                // This relies on the notificationId not wrapping around. This allows for
                // approximately 2,147,483,647 notifications, so it is a bit unlikely.
                // Otherwise we would need to store a 64bit counter instead.
                closeNotification(elem.key());

                // FIXME: compare index of event id of the read receipt and the notification instead
                // of just the id to prevent read receipts of events without notification clearing
                // all notifications in that room!
                if (elem.value() == reId)
                        break;
        }
}

void
NotificationsManager::actionInvoked(uint id, QString action)
{
        if (notificationIds.contains(id)) {
                roomEventId idEntry = notificationIds[id];
                if (action == "default") {
                        emit notificationClicked(idEntry.roomId, idEntry.eventId);
                }
        }
}

void
NotificationsManager::notificationReplied(uint id, QString reply)
{
        if (notificationIds.contains(id)) {
                roomEventId idEntry = notificationIds[id];
                emit sendNotificationReply(idEntry.roomId, idEntry.eventId, reply);
        }
}

void
NotificationsManager::notificationClosed(uint id, uint reason)
{
        Q_UNUSED(reason);
        notificationIds.remove(id);
}

/**
 * Automatic marshaling of a QImage for org.freedesktop.Notifications.Notify
 *
 * This function is from the Clementine project (see
 * http://www.clementine-player.org) and licensed under the GNU General Public
 * License, version 3 or later.
 *
 * Copyright 2010, David Sansome <me@davidsansome.com>
 */
QDBusArgument &
operator<<(QDBusArgument &arg, const QImage &image)
{
        if (image.isNull()) {
                arg.beginStructure();
                arg << 0 << 0 << 0 << false << 0 << 0 << QByteArray();
                arg.endStructure();
                return arg;
        }

        QImage scaled = image.scaledToHeight(100, Qt::SmoothTransformation);
        scaled        = scaled.convertToFormat(QImage::Format_ARGB32);

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        // ABGR -> ARGB
        QImage i = scaled.rgbSwapped();
#else
        // ABGR -> GBAR
        QImage i(scaled.size(), scaled.format());
        for (int y = 0; y < i.height(); ++y) {
                QRgb *p   = (QRgb *)scaled.scanLine(y);
                QRgb *q   = (QRgb *)i.scanLine(y);
                QRgb *end = p + scaled.width();
                while (p < end) {
                        *q = qRgba(qGreen(*p), qBlue(*p), qAlpha(*p), qRed(*p));
                        p++;
                        q++;
                }
        }
#endif

        arg.beginStructure();
        arg << i.width();
        arg << i.height();
        arg << i.bytesPerLine();
        arg << i.hasAlphaChannel();
        int channels = i.isGrayscale() ? 1 : (i.hasAlphaChannel() ? 4 : 3);
        arg << i.depth() / channels;
        arg << channels;
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
        arg << QByteArray(reinterpret_cast<const char *>(i.bits()), i.byteCount());
#else
        arg << QByteArray(reinterpret_cast<const char *>(i.bits()), i.sizeInBytes());
#endif
        arg.endStructure();
        return arg;
}

const QDBusArgument &
operator>>(const QDBusArgument &arg, QImage &)
{
        // This is needed to link but shouldn't be called.
        Q_ASSERT(0);
        return arg;
}
