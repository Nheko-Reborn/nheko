// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ThemeManager.h"

QColor
ThemeManager::themeColor(const QString &key) const
{
    if (key == QLatin1String("Black"))
        return QColor(0x17, 0x19, 0x19);

    else if (key == QLatin1String("BrightWhite"))
        return QColor(0xEB, 0xEB, 0xEB);
    else if (key == QLatin1String("FadedWhite"))
        return QColor(0xC9, 0xC9, 0xC9);
    else if (key == QLatin1String("MediumWhite"))
        return QColor(0x92, 0x92, 0x92);

    else if (key == QLatin1String("BrightGreen"))
        return QColor(0x1C, 0x31, 0x33);
    else if (key == QLatin1String("DarkGreen"))
        return QColor(0x57, 0x72, 0x75);
    else if (key == QLatin1String("LightGreen"))
        return QColor(0x46, 0xA4, 0x51);

    else if (key == QLatin1String("Gray"))
        return QColor(0x5D, 0x65, 0x65);
    else if (key == QLatin1String("Red"))
        return QColor(0xE2, 0x28, 0x26);
    else if (key == QLatin1String("Blue"))
        return QColor(0x81, 0xB3, 0xA9);

    else if (key == QLatin1String("Transparent"))
        return QColor(0, 0, 0, 0);

    return (QColor(0, 0, 0, 0));
}
