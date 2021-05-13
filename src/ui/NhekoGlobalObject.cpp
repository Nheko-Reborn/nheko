// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoGlobalObject.h"

#include "UserSettingsPage.h"

Nheko::Nheko()
{
        connect(
          UserSettings::instance().get(), &UserSettings::themeChanged, this, &Nheko::colorsChanged);
}

QPalette
Nheko::colors() const
{
        return QPalette();
}

QPalette
Nheko::inactiveColors() const
{
        QPalette p;
        p.setCurrentColorGroup(QPalette::ColorGroup::Inactive);
        return p;
}
