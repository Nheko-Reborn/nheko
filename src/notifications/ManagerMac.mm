#include "notifications/Manager.h"

#import <Foundation/Foundation.h>
#import <AppKit/NSImage.h>
#import <UserNotifications/UserNotifications.h>

#include <QtMac>
#include <QImage>

NotificationsManager::NotificationsManager(QObject *parent): QObject(parent)
{

}

void
NotificationsManager::objCxxPostNotification(const QString &room_name,
                                             const QString &room_id,
                                             const QString &event_id,
                                             const QString &subtitle,
                                             const QString &informativeText,
                                             const QString &bodyImagePath)
{
    UNAuthorizationOptions options = UNAuthorizationOptionAlert + UNAuthorizationOptionSound;
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];

    [center requestAuthorizationWithOptions:options
    completionHandler:^(BOOL granted, NSError * _Nullable error) {
        if (!granted) {
            NSLog(@"No notification access");
            if (error) {
                NSLog(@"%@",[error localizedDescription]);
            }
        }
    }];

    UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];

    content.title             = room_name.toNSString();
    content.subtitle          = subtitle.toNSString();
    content.body              = informativeText.toNSString();
    content.sound             = [UNNotificationSound defaultSound];
    content.threadIdentifier  = room_id.toNSString();

    if (!bodyImagePath.isEmpty()) {
        NSError * _Nullable error;
        NSURL *imageURL = [NSURL URLWithString:bodyImagePath.toNSString()];
        NSArray* attachments = [NSMutableArray array];
        UNNotificationAttachment *attachment = [UNNotificationAttachment attachmentWithIdentifier:@"" URL:imageURL options:nil error:&error];
        if (error) {
                NSLog(@"%@",[error localizedDescription]);
        }
        content.attachments = [attachments arrayByAddingObject:attachment];
    }

    UNNotificationRequest *notificationRequest = [UNNotificationRequest requestWithIdentifier:event_id.toNSString() content:content trigger:nil];

    [center addNotificationRequest:notificationRequest withCompletionHandler:^(NSError * _Nullable error) {
        if (error != nil) {
            NSLog(@"Unable to Add Notification Request");
        }
    }];

    [content autorelease];
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

