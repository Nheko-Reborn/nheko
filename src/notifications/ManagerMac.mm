#include "notifications/Manager.h"

#include <Foundation/Foundation.h>
#include <QtMac>

@interface NSUserNotification (CFIPrivate)
- (void)set_identityImage:(NSImage *)image;
@end

void
NotificationsManager::postNotification(const QString &roomName, const QString &userName, const QString &message)
{
    NSUserNotification * notif = [[NSUserNotification alloc] init];

    notif.title           = roomName.toNSString();
    notif.subtitle        = QString("%1 sent a message").arg(userName).toNSString();
    notif.informativeText = message.toNSString();
    notif.soundName       = NSUserNotificationDefaultSoundName;

    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification: notif];
    [notif autorelease];
}
