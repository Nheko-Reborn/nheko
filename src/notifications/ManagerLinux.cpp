// SPDX-FileCopyrightText: 2012 Roland Hieber <rohieb@rohieb.name>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
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
#include "UserSettingsPage.h"
#include "Utils.h"
#include "dbus/NhekoDBusApi.h"

NotificationsManager::NotificationsManager(QObject *parent)
  : QObject(parent)
  , dbus(QStringLiteral("org.freedesktop.Notifications"),
         QStringLiteral("/org/freedesktop/Notifications"),
         QStringLiteral("org.freedesktop.Notifications"),
         QDBusConnection::sessionBus(),
         this)
  , hasMarkup_{std::invoke([this]() -> bool {
      auto caps = dbus.call("GetCapabilities").arguments();
      for (const auto &x : qAsConst(caps))
          if (x.toStringList().contains("body-markup"))
              return true;
      return false;
  })}
  , hasImages_{std::invoke([this]() -> bool {
      auto caps = dbus.call("GetCapabilities").arguments();
      for (const auto &x : qAsConst(caps))
          if (x.toStringList().contains("body-images"))
              return true;
      return false;
  })}
{
    qDBusRegisterMetaType<QImage>();

    // clang-format off
    QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.Notifications"),
                                          QStringLiteral("/org/freedesktop/Notifications"),
                                          QStringLiteral("org.freedesktop.Notifications"),
                                          QStringLiteral("ActionInvoked"),
                                          this,
                                          SLOT(actionInvoked(uint,QString)));
    QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.Notifications"),
                                          QStringLiteral("/org/freedesktop/Notifications"),
                                          QStringLiteral("org.freedesktop.Notifications"),
                                          QStringLiteral("NotificationClosed"),
                                          this,
                                          SLOT(notificationClosed(uint,uint)));
    QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.Notifications"),
                                          QStringLiteral("/org/freedesktop/Notifications"),
                                          QStringLiteral("org.freedesktop.Notifications"),
                                          QStringLiteral("NotificationReplied"),
                                          this,
                                          SLOT(notificationReplied(uint,QString)));
    // clang-format on

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
    const auto room_id   = QString::fromStdString(notification.room_id);
    const auto event_id  = QString::fromStdString(mtx::accessors::event_id(notification.event));
    const auto room_name = QString::fromStdString(cache::singleRoomInfo(notification.room_id).name);

    const auto replaces_event_id =
      QString::fromStdString(mtx::accessors::relations(notification.event).replaces().value_or(""));

    auto postNotif = [this, room_id, event_id, room_name, icon, replaces_event_id](QString text) {
        if (replaces_event_id.isEmpty())
            emit systemPostNotificationCb(room_id, event_id, room_name, text, icon);
        else
            emit systemPostNotificationCb(room_id, replaces_event_id, room_name, text, icon);
    };

    QString template_ = getMessageTemplate(notification);
    // TODO: decrypt this message if the decryption setting is on in the UserSettings
    if (std::holds_alternative<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(
          notification.event)) {
        postNotif(template_);
        return;
    }

    if (hasMarkup_) {
        if (hasImages_ &&
            mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Image) {
            MxcImageProvider::download(
              QString::fromStdString(mtx::accessors::url(notification.event))
                .remove(QStringLiteral("mxc://")),
              QSize(200, 80),
              [postNotif, notification, template_](QString, QSize, QImage, QString imgPath) {
                  if (imgPath.isEmpty())
                      postNotif(template_
                                  .arg(utils::stripReplyFallbacks(notification.event, {}, {})
                                         .quoted_formatted_body)
                                  .replace(QLatin1String("<em>"), QLatin1String("<i>"))
                                  .replace(QLatin1String("</em>"), QLatin1String("</i>"))
                                  .replace(QLatin1String("<strong>"), QLatin1String("<b>"))
                                  .replace(QLatin1String("</strong>"), QLatin1String("</b>")));
                  else
                      postNotif(template_.arg(
                        QStringLiteral("<br><img src=\"file:///") % imgPath % "\" alt=\"" %
                        mtx::accessors::formattedBodyWithFallback(notification.event) % "\">"));
              });
            return;
        }

        postNotif(
          template_
            .arg(utils::stripReplyFallbacks(notification.event, {}, {}).quoted_formatted_body)
            .replace(QLatin1String("<em>"), QLatin1String("<i>"))
            .replace(QLatin1String("</em>"), QLatin1String("</i>"))
            .replace(QLatin1String("<strong>"), QLatin1String("<b>"))
            .replace(QLatin1String("</strong>"), QLatin1String("</b>")));
        return;
    }

    postNotif(template_.arg(utils::stripReplyFallbacks(notification.event, {}, {}).quoted_body));
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
    hints[QStringLiteral("image-data")]    = icon;
    hints[QStringLiteral("sound-name")]    = "message-new-instant";
    hints[QStringLiteral("desktop-entry")] = "nheko";
    hints[QStringLiteral("category")]      = "im.received";

    if (auto profile = UserSettings::instance()->profile(); !profile.isEmpty())
        hints[QStringLiteral("x-kde-origin-name")] = profile;

    uint replace_id = 0;
    if (!event_id.isEmpty()) {
        for (auto elem = notificationIds.begin(); elem != notificationIds.end(); ++elem) {
            if (elem.value().roomId != room_id)
                continue;

            if (elem.value().eventId == event_id) {
                replace_id = elem.key();
                break;
            }
        }
    }

    QList<QVariant> argumentList;
    argumentList << "nheko";          // app_name
    argumentList << (uint)replace_id; // replace_id
    argumentList << "";               // app_icon
    argumentList << roomName;         // summary
    argumentList << text;             // body

    // The list of actions has always the action name and then a localized version of that
    // action. Currently we just use an empty string for that.
    // TODO(Nico): Look into what to actually put there.
    argumentList << (QStringList(QStringLiteral("default"))
                     << QLatin1String("") << QStringLiteral("inline-reply")
                     << QLatin1String("")); // actions
    argumentList << hints;                  // hints
    argumentList << (int)-1;                // timeout in ms

    QDBusPendingCall call = dbus.asyncCallWithArgumentList(QStringLiteral("Notify"), argumentList);
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
    auto call    = dbus.asyncCall(QStringLiteral("CloseNotification"), (uint)id); // replace_id
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
        if (action == QLatin1String("default")) {
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
