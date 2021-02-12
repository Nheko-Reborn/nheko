#include "notifications/Manager.h"

#include <Foundation/Foundation.h>
#include <QtMac>

@interface NSUserNotification (CFIPrivate)
- (void)set_identityImage:(NSImage *)image;
@end

NotificationsManager::NotificationsManager(QObject *parent): QObject(parent)
{

}

void
NotificationsManager::postNotification(
                const QString &roomId,
                const QString &eventId,
                const QString &roomName,
                const QString &senderName,
                const QString &text,
                const QImage &icon,
                const bool &isEmoteMessage)
{
    Q_UNUSED(roomId);
    Q_UNUSED(eventId);
    Q_UNUSED(icon);

    NSUserNotification * notif = [[NSUserNotification alloc] init];

    notif.title           = roomName.toNSString();
    notif.subtitle        = QString("%1 sent a message").arg(senderName).toNSString();
    if (isEmoteMessage)
            notif.informativeText = QString("* ").append(senderName).append(" ").append(text).toNSString();
    else
            notif.informativeText = text.toNSString();
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

