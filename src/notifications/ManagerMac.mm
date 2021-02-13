#include "notifications/Manager.h"

#include <Foundation/Foundation.h>
#include <QtMac>

#include "Cache.h"
#include "EventAccessors.h"
#include "MatrixClient.h"
#include "Utils.h"
#include <mtx/responses/notifications.hpp>
#include <cmark.h>

@interface NSUserNotification (CFIPrivate)
- (void)set_identityImage:(NSImage *)image;
@end

NotificationsManager::NotificationsManager(QObject *parent): QObject(parent)
{

}

void
NotificationsManager::postNotification(const mtx::responses::Notification &notification,
                                       const QImage &icon)
{
    Q_UNUSED(icon);

    const auto sender   = cache::displayName(QString::fromStdString(notification.room_id), QString::fromStdString(mtx::accessors::sender(notification.event)));
    const auto text     = utils::event_body(notification.event);
    const auto formattedText = cmark_markdown_to_html(text.toStdString().c_str(), text.length(), CMARK_OPT_UNSAFE);

    NSUserNotification * notif = [[NSUserNotification alloc] init];

    notif.title           = QString::fromStdString(cache::singleRoomInfo(notification.room_id).name).toNSString();
    notif.subtitle        = QString("%1 sent a message").arg(sender).toNSString();
    if (mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Emote)
            notif.informativeText = QString("* ").append(sender).append(" ").append(formattedText).toNSString();
    else
            notif.informativeText = formattedText.toNSString();
    notif.soundName       = NSUserNotificationDefaultSoundName;

    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification: notif];
    [notif autorelease];
}

//unused
void
NotificationsManager::actionInvoked(uint, QString)
{
    }

void
NotificationsManager::notificationReplied(uint, QString)
{
}

void
NotificationsManager::notificationClosed(uint, uint)
{
}

void
NotificationsManager::removeNotification(const QString &, const QString &)
{}

