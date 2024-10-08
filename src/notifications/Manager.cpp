// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifications/Manager.h"

#include "Cache.h"
#include "Cache_p.h"
#include "EventAccessors.h"
#include "UserSettingsPage.h"
#include "Utils.h"

bool
NotificationsManager::allowShowingImages(const mtx::responses::Notification &notification)
{
    auto show = UserSettings::instance()->showImage();

    switch (show) {
    case UserSettings::ShowImage::Always:
        return true;
    case UserSettings::ShowImage::OnlyPrivate: {
        auto accessRules = cache::client()
                             ->getStateEvent<mtx::events::state::JoinRules>(notification.room_id)
                             .value_or(mtx::events::StateEvent<mtx::events::state::JoinRules>{})
                             .content;

        return accessRules.join_rule != mtx::events::state::JoinRule::Public;
    }
    case UserSettings::ShowImage::Never:
    default:
        return false;
    }
}

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

    bool containsSpoiler =
      mtx::accessors::formatted_body(notification.event).find("<span data-mx-spoiler") !=
      std::string::npos;

    if (containsSpoiler) {
        // Because we skip the %2 here, this might cause a warning in some cases.
        if (mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Emote) {
            return QStringLiteral("* %1 spoils something.").arg(sender);
        } else if (utils::isReply(notification.event)) {
            return tr("%1 replied with a spoiler.",
                      "Format a reply in a notification. %1 is the sender.")
              .arg(sender);
        } else {
            return QStringLiteral("%1 sent a spoiler.").arg(sender);
        }
    } else {
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

    for (const auto &[roomId, eventId] : std::as_const(this->notificationIds)) {
        if (roomId != roomId_)
            continue;
        auto idx = cache::getEventIndex(room_id, eventId.toStdString());
        if (!idx || markerPos >= idx) {
            removeNotification(roomId, eventId);
        }
    }
}

#include "moc_Manager.cpp"
