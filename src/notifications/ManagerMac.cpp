#include "Manager.h"

#include <QRegularExpression>
#include <QTextDocumentFragment>

#include "Cache.h"
#include "EventAccessors.h"
#include "Utils.h"

#include <mtx/responses/notifications.hpp>

#include <variant>

QString
NotificationsManager::formatNotification(const mtx::responses::Notification &notification)
{
        const auto sender =
          cache::displayName(QString::fromStdString(notification.room_id),
                             QString::fromStdString(mtx::accessors::sender(notification.event)));

        return QTextDocumentFragment::fromHtml(
                 mtx::accessors::formattedBodyWithFallback(notification.event)
                   .replace(QRegularExpression("<mx-reply>.+</mx-reply>"), ""))
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

        QImage *image = nullptr;
        if (mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Image)
                image = getImgOrNullptr(cacheImage(notification.event));

        const auto isEncrypted =
          std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(
            &notification.event) != nullptr;
        const auto isReply = utils::isReply(notification.event);

        if (isEncrypted) {
                // TODO: decrypt this message if the decryption setting is on in the UserSettings
                const QString messageInfo = (isReply ? tr("%1 replied with an encrypted message")
                                                     : tr("%1 sent an encrypted message"))
                                              .arg(sender);
                objCxxPostNotification(room_name, messageInfo, "", image);
        } else {
                const QString messageInfo =
                  (isReply ? tr("%1 replied to a message") : tr("%1 sent a message")).arg(sender);
                objCxxPostNotification(
                  room_name, messageInfo, formatNotification(notification), image);
        }
}

QImage *
NotificationsManager::getImgOrNullptr(const QString &path)
{
        auto img = new QImage{path};
        if (img->isNull()) {
                delete img;
                return nullptr;
        }
        return img;
}
