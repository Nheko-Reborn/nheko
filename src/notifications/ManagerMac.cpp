#include "Manager.h"

#include <QRegularExpression>
#include <QTextDocumentFragment>

#include "Cache.h"
#include "EventAccessors.h"
#include "Utils.h"

#include <mtx/responses/notifications.hpp>

QString
NotificationsManager::formatNotification(const mtx::responses::Notification &notification)
{
        const auto sender =
          cache::displayName(QString::fromStdString(notification.room_id),
                             QString::fromStdString(mtx::accessors::sender(notification.event)));

        return QTextDocumentFragment::fromHtml(
                 mtx::accessors::formattedBodyWithFallback(notification.event)
                   .replace(QRegularExpression("(<mx-reply>.+\\<\\/mx-reply\\>)"), ""))
          .toPlainText()
          .prepend((mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Emote)
                     ? "* " + sender + " "
                     : "");
}

void
NotificationsManager::postNotification(const mtx::responses::Notification &notification,
                                       const QImage &icon)
{
        Q_UNUSED(icon)

        const auto room_name =
          QString::fromStdString(cache::singleRoomInfo(notification.room_id).name);
        const auto sender =
          cache::displayName(QString::fromStdString(notification.room_id),
                             QString::fromStdString(mtx::accessors::sender(notification.event)));

        const QString messageInfo =
          QString("%1 %2 a message")
            .arg(sender)
            .arg((utils::isReply(notification.event)
                    ? tr("replied to",
                         "Used to denote that this message is a reply to another "
                         "message. Displayed as 'foo replied to a message'.")
                    : tr("sent",
                         "Used to denote that this message is a normal message. Displayed as 'foo "
                         "sent a message'.")));

        QString text = formatNotification(notification);

        QImage *image = nullptr;
        if (mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Image)
                image = new QImage{cacheImage(notification.event)};

        objCxxPostNotification(room_name, messageInfo, text, image);
}
