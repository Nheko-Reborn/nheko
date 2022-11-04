// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#import "notifications/MacNotificationDelegate.h"

#include <QString.h>

#include "ChatPage.h"

@implementation MacNotificationDelegate

- (id)initWithProxy: (std::unique_ptr<NotificationManagerProxy>&&)proxy
{
    if(self = [super init]) {
        mProxy = std::move(proxy);
    }

    return self;
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
    didReceiveNotificationResponse:(UNNotificationResponse*)response
             withCompletionHandler:(void (^)())completionHandler
{
    if ([response.actionIdentifier isEqualToString:@"ReplyAction"]) {
        if ([response respondsToSelector:@selector(userText)]) {
            UNTextInputNotificationResponse* textResponse = (UNTextInputNotificationResponse*)response;
            NSString* textValue = [textResponse userText];
            NSString* eventId = [[[textResponse notification] request] identifier];
            NSString* roomId = [[[[textResponse notification] request] content] threadIdentifier];
            mProxy->notificationReplied(QString::fromNSString(roomId), QString::fromNSString(eventId), QString::fromNSString(textValue));
        }
    }
    completionHandler();
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
       willPresentNotification:(UNNotification*)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler
{

    completionHandler(UNAuthorizationOptionAlert | UNAuthorizationOptionBadge | UNAuthorizationOptionSound);
}

@end