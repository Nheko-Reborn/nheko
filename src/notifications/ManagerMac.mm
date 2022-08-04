#include "notifications/Manager.h"

#import <Foundation/Foundation.h>
#import <AppKit/NSImage.h>
#import <UserNotifications/UserNotifications.h>

#include <QtMac>
#include <QImage>

@interface UNNotificationAttachment (UNNotificationAttachmentAdditions)
    + (UNNotificationAttachment *) createFromImageData:(NSData*)imgData identifier:(NSString *)imageFileIdentifier options:(NSDictionary*)attachmentOptions;
@end

@implementation UNNotificationAttachment (UNNotificationAttachmentAdditions)
    + (UNNotificationAttachment *) createFromImageData:(NSData*)imgData identifier:(NSString *)imageFileIdentifier options:(NSDictionary*)attachmentOptions {
        NSFileManager *fileManager = [NSFileManager defaultManager];
        NSString *tmpSubFolderName = [[NSProcessInfo processInfo] globallyUniqueString];
        NSURL *tmpSubFolderURL = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:tmpSubFolderName] isDirectory:true];
        NSError *error = nil;
        [fileManager createDirectoryAtURL:tmpSubFolderURL withIntermediateDirectories:true attributes:nil error:&error];
        if(error) {
            NSLog(@"%@",[error localizedDescription]);
            return nil;
        }
        NSURL *fileURL = [tmpSubFolderURL URLByAppendingPathComponent:imageFileIdentifier];
        [imgData writeToURL:fileURL atomically:true];
        UNNotificationAttachment *imageAttachment = [UNNotificationAttachment attachmentWithIdentifier:@"" URL:fileURL options:attachmentOptions error:&error];
        if(error) {
            NSLog(@"%@",[error localizedDescription]);
            return nil;
        }
        return imageAttachment;

    }
@end

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
    UNAuthorizationOptions options = UNAuthorizationOptionAlert + UNAuthorizationOptionSound + UNAuthorizationOptionBadge;
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
        NSURL *imageURL = [NSURL fileURLWithPath:bodyImagePath.toNSString()];
        NSData *img = [NSData dataWithContentsOfURL:imageURL];
        NSArray *attachments = [NSMutableArray array];
        UNNotificationAttachment *attachment = [UNNotificationAttachment createFromImageData:img identifier:@"attachment_image.jpeg" options:nil];
        if (attachment) {
            attachments = [NSMutableArray arrayWithObjects: attachment, nil];
            content.attachments = attachments;
        }
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

