// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFontDatabase>

#include "ThemeManager.h"

QColor
ThemeManager::themeColor(const QString &key) const
{
    if (key == "Black")
        return QColor(0x17, 0x19, 0x19);

    else if (key == "BrightWhite")
        return QColor(0xEB, 0xEB, 0xEB);
    else if (key == "FadedWhite")
        return QColor(0xC9, 0xC9, 0xC9);
    else if (key == "MediumWhite")
        return QColor(0x92, 0x92, 0x92);

    else if (key == "BrightGreen")
        return QColor(0x1C, 0x31, 0x33);
    else if (key == "DarkGreen")
        return QColor(0x57, 0x72, 0x75);
    else if (key == "LightGreen")
        return QColor(0x46, 0xA4, 0x51);

    else if (key == "Gray")
        return QColor(0x5D, 0x65, 0x65);
    else if (key == "Red")
        return QColor(0xE2, 0x28, 0x26);
    else if (key == "Blue")
        return QColor(0x81, 0xB3, 0xA9);

    else if (key == "Transparent")
        return QColor(0, 0, 0, 0);

    return (QColor(0, 0, 0, 0));
}
