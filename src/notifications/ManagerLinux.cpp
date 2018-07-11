#include "notifications/Manager.h"

#include <QImage>
#include <QDebug>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusConnection>

NotificationsManager::NotificationsManager(QObject *parent) :
    QObject(parent),
    dbus(
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        QDBusConnection::sessionBus(),
        this)
{
        qDBusRegisterMetaType<QImage>();

        //connectSlot("ActionInvoked", SLOT(actionInvoked(uint, QString)));
        //connectSlot("NotificationClosed", SLOT(notificationClosed(uint, uint)));
        QDBusConnection::sessionBus().connect(
                "org.freedesktop.Notifications",
                "/org/freedesktop/Notifications",
                "org.freedesktop.Notifications",
                "ActionInvoked",
                this,
                SLOT(actionInvoked(uint, QString)));
        QDBusConnection::sessionBus().connect(
                "org.freedesktop.Notifications",
                "/org/freedesktop/Notifications",
                "org.freedesktop.Notifications",
                "NotificationClosed",
                this,
                SLOT(notificationClosed(uint, uint)));
}

void
NotificationsManager::postNotification(const QString &roomid,
                                const QString &eventid,
                                const QString &roomname,
                                const QString &sender,
                                const QString &text,
                                const QImage &icon)
{
    uint id = showNotification(roomname, sender+": "+text, icon);
    notificationIds[id] = roomEventId{roomid,eventid};
}
/**
 * This function is based on code from
 * https://github.com/rohieb/StratumsphereTrayIcon
 * Copyright (C) 2012 Roland Hieber <rohieb@rohieb.name>
 * Licensed under the GNU General Public License, version 3
 */
uint
NotificationsManager::showNotification(const QString summary, const QString text, const QImage image)
{
        QVariantMap hints;
        hints["image_data"] = image;
        QList<QVariant> argumentList;
        argumentList << "nheko"; //app_name
        argumentList << (uint)0;  // replace_id
        argumentList << "";  // app_icon
        argumentList << summary; // summary
        argumentList << text; // body
        argumentList << (QStringList("default")<<"reply");  // actions
        argumentList << hints;  // hints
        argumentList << (int)0; // timeout in ms

        static QDBusInterface notifyApp(
                    "org.freedesktop.Notifications",
                    "/org/freedesktop/Notifications",
                    "org.freedesktop.Notifications");
        QDBusMessage reply = notifyApp.callWithArgumentList(
                    QDBus::AutoDetect,
                    "Notify",
                    argumentList);
        if(reply.type() == QDBusMessage::ErrorMessage) {
            qDebug() << "D-Bus Error:" << reply.errorMessage();
            return 0;
        } else {
            return reply.arguments().first().toUInt();
        }
        return true;
}

void
NotificationsManager::actionInvoked(uint id, QString action)
{
        if (action == "default" && notificationIds.contains(id)) {
                roomEventId idEntry = notificationIds[id];
                emit notificationClicked(idEntry.roomId, idEntry.eventId);
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
QDBusArgument& operator<<(QDBusArgument& arg, const QImage& image) {
  if(image.isNull()) {
    arg.beginStructure();
    arg << 0 << 0 << 0 << false << 0 << 0 << QByteArray();
    arg.endStructure();
    return arg;
  }

  QImage scaled = image.scaledToHeight(100, Qt::SmoothTransformation);
  scaled = scaled.convertToFormat(QImage::Format_ARGB32);

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
  // ABGR -> ARGB
  QImage i = scaled.rgbSwapped();
#else
  // ABGR -> GBAR
  QImage i(scaled.size(), scaled.format());
  for (int y = 0; y < i.height(); ++y) {
    QRgb* p = (QRgb*) scaled.scanLine(y);
    QRgb* q = (QRgb*) i.scanLine(y);
    QRgb* end = p + scaled.width();
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
  arg << QByteArray(reinterpret_cast<const char*>(i.bits()), i.byteCount());
  arg.endStructure();
  return arg;
}

const QDBusArgument& operator>>(const QDBusArgument& arg, QImage&) {
  // This is needed to link but shouldn't be called.
  Q_ASSERT(0);
  return arg;
}
