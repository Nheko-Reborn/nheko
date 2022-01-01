// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Theme.h"

Q_DECLARE_METATYPE(Theme)

QPalette
Theme::paletteFromTheme(QStringView theme)
{
    [[maybe_unused]] static auto meta = qRegisterMetaType<Theme>("Theme");
    static QPalette original;
    if (theme == u"light") {
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
    } else if (theme == u"dark") {
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

Theme::Theme(QStringView theme)
{
    auto p     = paletteFromTheme(theme);
    separator_ = p.mid().color();
    if (theme == u"light") {
        sidebarBackground_ = QColor(0x23, 0x36, 0x49);
        alternateButton_   = QColor(0xcc, 0xcc, 0xcc);
        red_               = QColor(0xa8, 0x23, 0x53);
        orange_            = QColor(0xfc, 0xbe, 0x05);
    } else if (theme == u"dark") {
        sidebarBackground_ = QColor(0x2d, 0x31, 0x39);
        alternateButton_   = QColor(0x41, 0x4A, 0x59);
        red_               = QColor(0xa8, 0x23, 0x53);
        orange_            = QColor(0xfc, 0xc5, 0x3a);
    } else {
        sidebarBackground_ = p.window().color();
        alternateButton_   = p.dark().color();
        red_               = QColor(Qt::GlobalColor::red);
        orange_            = QColor(0xff, 0xa5, 0x00); // SVG orange
    }
}
