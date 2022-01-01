// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QCommonStyle>

class ThemeManager : public QCommonStyle
{
    Q_OBJECT

public:
    inline static ThemeManager &instance();

    QColor themeColor(const QString &key) const;

private:
    ThemeManager() {}

    ThemeManager(ThemeManager const &);
    void operator=(ThemeManager const &);
};

inline ThemeManager &
ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}
