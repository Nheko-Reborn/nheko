// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QPalette>
#include <QQmlEngine>
#include <QString>
#include <QStringList>

#include <optional>

// A theme loaded from ~/.local/share/nheko/themes/<id>.json (or the equivalent
// GenericDataLocation on other platforms), rather than one of the built-in
// light/dark/system/tokyonight themes hardcoded in Theme.cpp. Every color is required - unlike
// the built-ins, there's no partial/inherited theme support, so a file missing or misspelling a
// key just fails to load entirely (logged, not applied).
struct CustomTheme
{
    QString displayName;
    QPalette palette;
    QColor sidebarBackground, alternateButton, red, green, orange, error;
};

namespace ThemeLoader {
// Ids (filenames minus ".json") of every theme file found in the themes directory, sorted.
QStringList customThemeIds();
// The theme's own "name" field if it loads successfully, otherwise just id unchanged.
QString displayName(const QString &id);
// Parses <themes dir>/<id>.json. Cached after the first call for a given id - a file edited or
// added after startup needs a restart to be picked up.
std::optional<CustomTheme> load(const QString &id);
}

class Theme final : public QPalette
{
    Q_GADGET
    QML_ANONYMOUS

    Q_PROPERTY(QColor sidebarBackground READ sidebarBackground CONSTANT)
    Q_PROPERTY(QColor alternateButton READ alternateButton CONSTANT)
    Q_PROPERTY(QColor separator READ separator CONSTANT)
    Q_PROPERTY(QColor red READ red CONSTANT)
    Q_PROPERTY(QColor green READ green CONSTANT)
    Q_PROPERTY(QColor error READ error CONSTANT)
    Q_PROPERTY(QColor orange READ orange CONSTANT)
    Q_PROPERTY(QColor online READ online CONSTANT)
    Q_PROPERTY(QColor unavailable READ unavailable CONSTANT)
public:
    Theme() {}
    explicit Theme(QStringView theme);
    static QPalette paletteFromTheme(QStringView theme);

    QColor sidebarBackground() const { return sidebarBackground_; }
    QColor alternateButton() const { return alternateButton_; }
    QColor separator() const { return separator_; }
    QColor red() const { return red_; }
    QColor green() const { return green_; }
    QColor error() const { return error_; }
    QColor orange() const { return orange_; }
    QColor online() const { return QColor(0x00, 0xcc, 0x66); }
    QColor unavailable() const { return QColor(0xff, 0x99, 0x33); }

private:
    QColor sidebarBackground_, separator_, red_, green_, error_, orange_, alternateButton_;
};
