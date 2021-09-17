// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifications/Manager.h"

#include "Cache.h"
#include "EventAccessors.h"
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
        return tr("* %1 %2",
                  "Format an emote message in a notification, %1 is the sender, %2 the "
                  "message")
          .arg(sender);
    } else if (utils::isReply(notification.event)) {
        return tr("%1 replied: %2",
                  "Format a reply in a notification. %1 is the sender, %2 the message")
          .arg(sender);
    } else {
        return tr("%1: %2",
                  "Format a normal message in a notification. %1 is the sender, %2 the "
                  "message")
          .arg(sender);
    }
}
