#include "notifications/Manager.h"

#import <Foundation/Foundation.h>
#import <AppKit/NSImage.h>

#include <QtMac>
#include <QImage>

@interface NSUserNotification (CFIPrivate)
- (void)set_identityImage:(NSImage *)image;
@end

NotificationsManager::NotificationsManager(QObject *parent): QObject(parent)
{

}

void
NotificationsManager::objCxxPostNotification(const QString &title,
                                             const QString &subtitle,
                                             const QString &informativeText,
                                             const QImage &bodyImage)
{

    NSUserNotification *notif = [[NSUserNotification alloc] init];

    notif.title           = title.toNSString();
    notif.subtitle        = subtitle.toNSString();
    notif.informativeText = informativeText.toNSString();
    notif.soundName       = NSUserNotificationDefaultSoundName;

    if (!bodyImage.isNull())
        notif.contentImage = [[NSImage alloc] initWithCGImage: bodyImage.toCGImage() size: NSZeroSize];

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

