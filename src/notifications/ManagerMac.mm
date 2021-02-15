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
NotificationsManager::systemPostNotification(const QString &room_id,
                                             const QString &event_id,
                                             const QString &roomName,
                                             const QString &sender,
                                             const QString &text,
                                             const QImage &icon)
{
    Q_UNUSED(room_id)
    Q_UNUSED(event_id)
    Q_UNUSED(icon)

    NSUserNotification * notif = [[NSUserNotification alloc] init];

    notif.title           = roomName.toNSString();
    notif.subtitle        = QString("%1 sent a message").arg(sender).toNSString();
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

