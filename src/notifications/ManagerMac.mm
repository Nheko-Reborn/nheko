// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifications/NotificationManagerProxy.h"
#include "notifications/MacNotificationDelegate.h"
#include "notifications/Manager.h"

#include "ChatPage.h"

#import <AppKit/NSImage.h>
#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>

#include <QImage>
#include <QtMac>

@interface UNNotificationAttachment (UNNotificationAttachmentAdditions)
+ (UNNotificationAttachment*)createFromImageData:(NSData*)imgData
                                      identifier:(NSString*)imageFileIdentifier
                                         options:
                                             (NSDictionary*)attachmentOptions;
@end

@implementation UNNotificationAttachment (UNNotificationAttachmentAdditions)
+ (UNNotificationAttachment*)createFromImageData:(NSData*)imgData
                                      identifier:(NSString*)imageFileIdentifier
                                         options:
                                             (NSDictionary*)attachmentOptions
{
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSString* tmpSubFolderName =
        [[NSProcessInfo processInfo] globallyUniqueString];
    NSURL* tmpSubFolderURL = [NSURL
        fileURLWithPath:[NSTemporaryDirectory()
                            stringByAppendingPathComponent:tmpSubFolderName]
            isDirectory:true];
    NSError* error = nil;
    [fileManager createDirectoryAtURL:tmpSubFolderURL
          withIntermediateDirectories:true
                           attributes:nil
                                error:&error];
    if (error) {
        NSLog(@"%@", [error localizedDescription]);
        return nil;
    }
    NSURL* fileURL =
        [tmpSubFolderURL URLByAppendingPathComponent:imageFileIdentifier];
    [imgData writeToURL:fileURL atomically:true];
    UNNotificationAttachment* imageAttachment =
        [UNNotificationAttachment attachmentWithIdentifier:@""
                                                       URL:fileURL
                                                   options:attachmentOptions
                                                     error:&error];
    if (error) {
        NSLog(@"%@", [error localizedDescription]);
        return nil;
    }
    return imageAttachment;
}
@end

NotificationsManager::NotificationsManager(QObject* parent)
    : QObject(parent)
{
}

void NotificationsManager::objCxxPostNotification(
    const QString& room_name,
    const QString& room_id,
    const QString& event_id,
    const QString& subtitle,
    const QString& informativeText,
    const QString& bodyImagePath,
    const QString& respondStr,
    const QString& sendStr,
    const QString& placeholder,
    const bool enableSound)
{
    // Request permissions for alerts (the generic type of notification), sound playback,
    // and badges (which allows the Nheko app icon to show the little red bubble with unread count).
    // NOTE: Possible macOS bug... the 'Play sound for notification checkbox' doesn't appear in
    // the Notifications and Focus settings unless UNAuthorizationOptionBadges is also
    // specified
    UNAuthorizationOptions options = UNAuthorizationOptionAlert + UNAuthorizationOptionSound + UNAuthorizationOptionBadge;
    UNUserNotificationCenter* center =
        [UNUserNotificationCenter currentNotificationCenter];

    // TODO: Move this somewhere that isn't dependent on receiving a notification
    // to actually request notification access.
    [center requestAuthorizationWithOptions:options
                          completionHandler:^(BOOL granted,
                              NSError* _Nullable error) {
                              if (!granted) {
                                  NSLog(@"No notification access");
                                  if (error) {
                                      NSLog(@"%@", [error localizedDescription]);
                                  }
                              }
                          }];

    UNTextInputNotificationAction* replyAction = [UNTextInputNotificationAction actionWithIdentifier:@"ReplyAction"
                                                                                               title:respondStr.toNSString()
                                                                                             options:UNNotificationActionOptionNone
                                                                                textInputButtonTitle:sendStr.toNSString()
                                                                                textInputPlaceholder:placeholder.toNSString()];

    UNNotificationCategory* category = [UNNotificationCategory categoryWithIdentifier:@"ReplyCategory"
                                                                              actions:@[ replyAction ]
                                                                    intentIdentifiers:@[]
                                                                              options:UNNotificationCategoryOptionNone];

    NSString* title = room_name.toNSString();
    NSString* sub = subtitle.toNSString();
    NSString* body = informativeText.toNSString();
    NSString* threadIdentifier = room_id.toNSString();
    NSString* identifier = event_id.toNSString();
    NSString* imgUrl = bodyImagePath.toNSString();

    NSSet* categories = [NSSet setWithObject:category];
    [center setNotificationCategories:categories];
    [center getNotificationSettingsWithCompletionHandler:^(
        UNNotificationSettings* _Nonnull settings) {
        if (settings.authorizationStatus == UNAuthorizationStatusAuthorized) {
            UNMutableNotificationContent* content =
                [[UNMutableNotificationContent alloc] init];

            content.title = title;
            content.subtitle = sub;
            content.body = body;
            if (enableSound) {
                content.sound = [UNNotificationSound defaultSound];
            }
            content.threadIdentifier = threadIdentifier;
            content.categoryIdentifier = @"ReplyCategory";

            if ([imgUrl length] != 0) {
                NSURL* imageURL = [NSURL fileURLWithPath:imgUrl];
                NSData* img = [NSData dataWithContentsOfURL:imageURL];
                NSArray* attachments = [NSMutableArray array];
                UNNotificationAttachment* attachment = [UNNotificationAttachment
                    createFromImageData:img
                             identifier:@"attachment_image.jpeg"
                                options:nil];
                if (attachment) {
                    attachments = [NSMutableArray arrayWithObjects:attachment, nil];
                    content.attachments = attachments;
                }
            }

            UNNotificationRequest* notificationRequest =
                [UNNotificationRequest requestWithIdentifier:identifier
                                                     content:content
                                                     trigger:nil];

            [center addNotificationRequest:notificationRequest
                     withCompletionHandler:^(NSError* _Nullable error) {
                         if (error != nil) {
                             NSLog(@"Unable to Add Notification Request: %@", [error localizedDescription]);
                         }
                     }];

            [content autorelease];
        }
    }];
}

void NotificationsManager::attachToMacNotifCenter()
{
    UNUserNotificationCenter* center =
        [UNUserNotificationCenter currentNotificationCenter];

    std::unique_ptr<NotificationManagerProxy> proxy = std::make_unique<NotificationManagerProxy>();

    connect(proxy.get(), &NotificationManagerProxy::notificationReplied, ChatPage::instance(), &ChatPage::sendNotificationReply);

    MacNotificationDelegate* notifDelegate = [[MacNotificationDelegate alloc] initWithProxy:std::move(proxy)];

    center.delegate = notifDelegate;
}

// unused
void NotificationsManager::actionInvoked(uint, QString) { }

void NotificationsManager::notificationReplied(uint, QString) { }

void NotificationsManager::notificationClosed(uint, uint) { }

void NotificationsManager::removeNotification(const QString&, const QString&) { }
