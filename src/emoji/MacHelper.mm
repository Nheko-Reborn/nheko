#include "MacHelper.h"

#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <Foundation/NSString.h>
#include <QCoreApplication>

void
MacHelper::showEmojiWindow()
{
        NSApplication *theNSApplication = [NSApplication sharedApplication];
        [theNSApplication orderFrontCharacterPalette:nil];
}

void
MacHelper::initializeMenus()
{
        NSApplication *theNSApplication = [NSApplication sharedApplication];

        NSArray<NSMenuItem *> *menus = [theNSApplication mainMenu].itemArray;
        NSUInteger size              = menus.count;
        for (NSUInteger i = 0; i < size; i++) {
                NSMenuItem *item = [menus objectAtIndex:i];
                [item setTitle:@"Edit"];
        }
}
