// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Emoji.h"

#include <QCoreApplication>
QString
emoji::categoryToName(emoji::Emoji::Category cat)
{
    switch (cat) {
    case emoji::Emoji::Category::People:
        return QCoreApplication::translate("emoji-catagory", "People");
    case emoji::Emoji::Category::Nature:
        return QCoreApplication::translate("emoji-catagory", "Nature");
    case emoji::Emoji::Category::Food:
        return QCoreApplication::translate("emoji-catagory", "Food");
    case emoji::Emoji::Category::Activity:
        return QCoreApplication::translate("emoji-catagory", "Activity");
    case emoji::Emoji::Category::Travel:
        return QCoreApplication::translate("emoji-catagory", "Travel");
    case emoji::Emoji::Category::Objects:
        return QCoreApplication::translate("emoji-catagory", "Objects");
    case emoji::Emoji::Category::Symbols:
        return QCoreApplication::translate("emoji-catagory", "Symbols");
    case emoji::Emoji::Category::Flags:
        return QCoreApplication::translate("emoji-catagory", "Flags");
    default:
        return "";
    }
}

#include "moc_Emoji.cpp"
