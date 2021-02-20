#include "notifications/Manager.h"

#include "Cache.h"
#include "EventAccessors.h"
#include "Utils.h"
#include <mtx/responses/notifications.hpp>

void
NotificationsManager::postNotification(const mtx::responses::Notification &notification,
                                       const QImage &icon)
{
        const auto room_id  = QString::fromStdString(notification.room_id);
        const auto event_id = QString::fromStdString(mtx::accessors::event_id(notification.event));
        const auto room_name =
          QString::fromStdString(cache::singleRoomInfo(notification.room_id).name);
        const auto sender = cache::displayName(
          room_id, QString::fromStdString(mtx::accessors::sender(notification.event)));

        const QString reply = (utils::isReply(notification.event)
                                 ? ""
                                 : tr(" replied",
                                      "Used to denote that this message is a reply to another "
                                      "message. Displayed as 'foo replied: message'."));

        // the "replied" is only added if this message is not an emote message
        QString text =
          ((mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Emote)
             ? "* " + sender + " "
             : sender + reply + ": ") +
          formatNotification(notification.event);

        systemPostNotification(room_id, event_id, room_name, sender, text, icon);
}
