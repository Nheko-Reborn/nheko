// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFontDatabase>

#include "ThemeManager.h"

ThemeManager::ThemeManager() {}

QColor
ThemeManager::themeColor(const QString &key) const
{
    if (key == "Black")
        return QColor("#171919");

    else if (key == "BrightWhite")
        return QColor("#EBEBEB");
    else if (key == "FadedWhite")
        return QColor("#C9C9C9");
    else if (key == "MediumWhite")
        return QColor("#929292");

    else if (key == "BrightGreen")
        return QColor("#1C3133");
    else if (key == "DarkGreen")
        return QColor("#577275");
    else if (key == "LightGreen")
        return QColor("#46A451");

    else if (key == "Gray")
        return QColor("#5D6565");
    else if (key == "Red")
        return QColor("#E22826");
    else if (key == "Blue")
        return QColor("#81B3A9");

    else if (key == "Transparent")
        return QColor(0, 0, 0, 0);

    return (QColor(0, 0, 0, 0));
}
