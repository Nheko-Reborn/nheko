// SPDX-FileCopyrightText: 2012 Roland Hieber <rohieb@rohieb.name>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifications/Manager.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>
#include <QImage>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QTextDocumentFragment>

#include <functional>
#include <variant>

#include <mtx/responses/notifications.hpp>

#include "Cache.h"
#include "EventAccessors.h"
#include "MxcImageProvider.h"
#include "Utils.h"

NotificationsManager::NotificationsManager(QObject *parent)
  : QObject(parent)
  , dbus("org.freedesktop.Notifications",
         "/org/freedesktop/Notifications",
         "org.freedesktop.Notifications",
         QDBusConnection::sessionBus(),
         this)
  , hasMarkup_{std::invoke([this]() -> bool {
          for (auto x : dbus.call("GetCapabilities").arguments())
                  if (x.toStringList().contains("body-markup"))
                          return true;
          return false;
  })}
  , hasImages_{std::invoke([this]() -> bool {
          for (auto x : dbus.call("GetCapabilities").arguments())
                  if (x.toStringList().contains("body-images"))
                          return true;
          return false;
  })}
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

        connect(this,
                &NotificationsManager::systemPostNotificationCb,
                this,
                &NotificationsManager::systemPostNotification,
                Qt::QueuedConnection);
}

void
NotificationsManager::postNotification(const mtx::responses::Notification &notification,
                                       const QImage &icon)
{
        const auto room_id  = QString::fromStdString(notification.room_id);
        const auto event_id = QString::fromStdString(mtx::accessors::event_id(notification.event));
        const auto room_name =
          QString::fromStdString(cache::singleRoomInfo(notification.room_id).name);

        auto postNotif = [this, room_id, event_id, room_name, icon](QString text) {
                emit systemPostNotificationCb(room_id, event_id, room_name, text, icon);
        };

        QString template_ = getMessageTemplate(notification);
        // TODO: decrypt this message if the decryption setting is on in the UserSettings
        if (std::holds_alternative<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(
              notification.event)) {
                postNotif(template_);
                return;
        }

        if (hasMarkup_) {
                if (hasImages_ && mtx::accessors::msg_type(notification.event) ==
                                    mtx::events::MessageType::Image) {
                        MxcImageProvider::download(
                          QString::fromStdString(mtx::accessors::url(notification.event))
                            .remove("mxc://"),
                          QSize(200, 80),
                          [postNotif, notification, template_](
                            QString, QSize, QImage, QString imgPath) {
                                  if (imgPath.isEmpty())
                                          postNotif(template_
                                                      .arg(utils::stripReplyFallbacks(
                                                             notification.event, {}, {})
                                                             .quoted_formatted_body)
                                                      .replace("<em>", "<i>")
                                                      .replace("</em>", "</i>")
                                                      .replace("<strong>", "<b>")
                                                      .replace("</strong>", "</b>"));
                                  else
                                          postNotif(template_.arg(
                                            QStringLiteral("<br><img src=\"file:///") % imgPath %
                                            "\" alt=\"" %
                                            mtx::accessors::formattedBodyWithFallback(
                                              notification.event) %
                                            "\">"));
                          });
                        return;
                }

                postNotif(
                  template_
                    .arg(
                      utils::stripReplyFallbacks(notification.event, {}, {}).quoted_formatted_body)
                    .replace("<em>", "<i>")
                    .replace("</em>", "</i>")
                    .replace("<strong>", "<b>")
                    .replace("</strong>", "</b>"));
                return;
        }

        postNotif(
          template_.arg(utils::stripReplyFallbacks(notification.event, {}, {}).quoted_body));
}

/**
 * This function is based on code from
 * https://github.com/rohieb/StratumsphereTrayIcon
 * Copyright (C) 2012 Roland Hieber <rohieb@rohieb.name>
 * Licensed under the GNU General Public License, version 3
 */
void
NotificationsManager::systemPostNotification(const QString &room_id,
                                             const QString &event_id,
                                             const QString &roomName,
                                             const QString &text,
                                             const QImage &icon)
{
        QVariantMap hints;
        hints["image-data"] = icon;
        hints["sound-name"] = "message-new-instant";
        QList<QVariant> argumentList;
        argumentList << "nheko";  // app_name
        argumentList << (uint)0;  // replace_id
        argumentList << "";       // app_icon
        argumentList << roomName; // summary
        argumentList << text;     // body

        // The list of actions has always the action name and then a localized version of that
        // action. Currently we just use an empty string for that.
        // TODO(Nico): Look into what to actually put there.
        argumentList << (QStringList("default") << ""
                                                << "inline-reply"
                                                << ""); // actions
        argumentList << hints;                          // hints
        argumentList << (int)-1;                        // timeout in ms

        QDBusPendingCall call = dbus.asyncCallWithArgumentList("Notify", argumentList);
        auto watcher          = new QDBusPendingCallWatcher{call, this};
        connect(
          watcher, &QDBusPendingCallWatcher::finished, this, [watcher, this, room_id, event_id]() {
                  if (watcher->reply().type() == QDBusMessage::ErrorMessage)
                          qDebug() << "D-Bus Error:" << watcher->reply().errorMessage();
                  else
                          notificationIds[watcher->reply().arguments().first().toUInt()] =
                            roomEventId{room_id, event_id};
                  watcher->deleteLater();
          });
}

void
NotificationsManager::closeNotification(uint id)
{
        auto call    = dbus.asyncCall("CloseNotification", (uint)id); // replace_id
        auto watcher = new QDBusPendingCallWatcher{call, this};
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher]() {
                if (watcher->reply().type() == QDBusMessage::ErrorMessage) {
                        qDebug() << "D-Bus Error:" << watcher->reply().errorMessage();
                };
                watcher->deleteLater();
        });
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
 * SPDX-FileCopyrightText: 2010 David Sansome <me@davidsansome.com>
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
