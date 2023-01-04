// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifications/Manager.h"

#include "Cache.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "Utils.h"

QString
NotificationsManager::getMessageTemplate(const mtx::responses::Notification &notification)
{
    const auto sender =
      cache::displayName(QString::fromStdString(notification.room_id),
                         QString::fromStdString(mtx::accessors::sender(notification.event)));

    // TODO: decrypt this message if the decryption setting is on in the UserSettings
    if (auto msg = std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(
          &notification.event);
        msg != nullptr) {
        return tr("%1 sent an encrypted message").arg(sender);
    }

    if (mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Emote) {
        return QStringLiteral("* %1 %2").arg(sender);
    } else if (utils::isReply(notification.event)) {
        return tr("%1 replied: %2",
                  "Format a reply in a notification. %1 is the sender, %2 the message")
          .arg(sender);
    } else {
        return QStringLiteral("%1: %2").arg(sender);
    }
}

void
NotificationsManager::removeNotifications(const QString &roomId_,
                                          const std::vector<QString> &eventIds)
{
    std::string room_id = roomId_.toStdString();

    std::uint64_t markerPos = 0;
    for (const auto &e : eventIds) {
        markerPos = std::max(markerPos, cache::getEventIndex(room_id, e.toStdString()).value_or(0));
    }

    for (const auto &[roomId, eventId] : qAsConst(this->notificationIds)) {
        if (roomId != roomId_)
            continue;
        auto idx = cache::getEventIndex(room_id, eventId.toStdString());
        if (!idx || markerPos >= idx) {
            removeNotification(roomId, eventId);
        }
    }
}
