// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "notifications/Manager.h"
#include "notifications/NotificationManagerProxy.h"
#include <mtx/responses/notifications.hpp>

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>

@interface MacNotificationDelegate : NSObject <UNUserNotificationCenterDelegate> {
    std::unique_ptr<NotificationManagerProxy> mProxy;
}

- (id)initWithProxy:(std::unique_ptr<NotificationManagerProxy>&&)proxy;
@end
