// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Theme.h"

QPalette
Theme::paletteFromTheme(Theme::Kind theme)
{
    static QPalette original;
    if (theme == Kind::Light) {
        static QPalette lightActive = [] {
            QPalette lightActive(
              /*windowText*/ QColor(0x33, 0x33, 0x33),
              /*button*/ QColor(Qt::GlobalColor::white),
              /*light*/ QColor(0xef, 0xef, 0xef),
              /*dark*/ QColor(70, 77, 93),
              /*mid*/ QColor(220, 220, 220),
              /*text*/ QColor(0x33, 0x33, 0x33),
              /*bright_text*/ QColor(0xf2, 0xf5, 0xf8),
              /*base*/ QColor(Qt::GlobalColor::white),
              /*window*/ QColor(Qt::GlobalColor::white));
            lightActive.setColor(QPalette::AlternateBase, QColor(0xee, 0xee, 0xee));
            lightActive.setColor(QPalette::Highlight, QColor(0x38, 0xa3, 0xd8));
            lightActive.setColor(QPalette::HighlightedText, QColor(0xf4, 0xf4, 0xf5));
            lightActive.setColor(QPalette::ToolTipBase, lightActive.base().color());
            lightActive.setColor(QPalette::ToolTipText, lightActive.text().color());
            lightActive.setColor(QPalette::Link, QColor(0x00, 0x77, 0xb5));
            lightActive.setColor(QPalette::ButtonText, QColor(0x55, 0x54, 0x59));
            return lightActive;
        }();
        return lightActive;
    } else if (theme == Kind::Dark) {
        static QPalette darkActive = [] {
            QPalette darkActive(
              /*windowText*/ QColor(0xca, 0xcc, 0xd1),
              /*button*/ QColor(Qt::GlobalColor::white),
              /*light*/ QColor(0xca, 0xcc, 0xd1),
              /*dark*/ QColor(60, 70, 77),
              /*mid*/ QColor(0x20, 0x22, 0x28),
              /*text*/ QColor(0xca, 0xcc, 0xd1),
              /*bright_text*/ QColor(0xf4, 0xf5, 0xf8),
              /*base*/ QColor(0x20, 0x22, 0x28),
              /*window*/ QColor(0x2d, 0x31, 0x39));
            darkActive.setColor(QPalette::AlternateBase, QColor(0x2d, 0x31, 0x39));
            darkActive.setColor(QPalette::Highlight, QColor(0x38, 0xa3, 0xd8));
            darkActive.setColor(QPalette::HighlightedText, QColor(0xf4, 0xf5, 0xf8));
            darkActive.setColor(QPalette::ToolTipBase, darkActive.base().color());
            darkActive.setColor(QPalette::ToolTipText, darkActive.text().color());
            darkActive.setColor(QPalette::Link, QColor(0x38, 0xa3, 0xd8));
            darkActive.setColor(QPalette::ButtonText, QColor(0x82, 0x82, 0x84));
            return darkActive;
        }();
        return darkActive;
    } else {
        return original;
    }
}

QPalette
Theme::paletteFromTheme(QStringView theme)
{
    return paletteFromTheme(kindFromString(theme));
}

Theme::Theme(Theme::Kind theme)
{
    auto p     = paletteFromTheme(theme);
    separator_ = p.mid().color();
    if (theme == Kind::Light) {
        sidebarBackground_ = QColor(0x23, 0x36, 0x49);
        red_               = QColor(0xa8, 0x23, 0x53);
        green_             = QColor(QColorConstants::Svg::green);
        orange_            = QColor(0xfc, 0xbe, 0x05);
        error_             = QColor(0xdd, 0x3d, 0x3d);
    } else if (theme == Kind::Dark) {
        sidebarBackground_ = QColor(0x2d, 0x31, 0x39);
        red_               = QColor(0xa8, 0x23, 0x53);
        green_             = QColor(QColorConstants::Svg::green);
        orange_            = QColor(0xfc, 0xc5, 0x3a);
        error_             = QColor(0xdd, 0x3d, 0x3d);
    } else {
        sidebarBackground_ = p.window().color();
        red_               = QColor(QColorConstants::Svg::red);
        green_             = QColor(QColorConstants::Svg::green);
        orange_            = QColor(QColorConstants::Svg::orange); // SVG orange
        error_             = QColor(0xdd, 0x3d, 0x3d);
    }
}

Theme::Kind
Theme::kindFromString(QStringView kind)
{
    if (kind == u"light") {
        return Kind::Light;
    } else if (kind == u"dark") {
        return Kind::Dark;
    } else if (kind == u"system") {
        return Kind::System;
    } else {
        throw std::invalid_argument("Unknown theme kind: " + kind.toString().toStdString());
    }
}

#include "moc_Theme.cpp"
