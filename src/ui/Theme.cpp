// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Theme.h"

Q_DECLARE_METATYPE(Theme)

QPalette
Theme::paletteFromTheme(std::string_view theme)
{
        [[maybe_unused]] static auto meta = qRegisterMetaType<Theme>("Theme");
        static QPalette original;
        if (theme == "light") {
                QPalette lightActive(
                  /*windowText*/ QColor("#333"),
                  /*button*/ QColor("white"),
                  /*light*/ QColor(0xef, 0xef, 0xef),
                  /*dark*/ QColor(70, 77, 93),
                  /*mid*/ QColor(220, 220, 220),
                  /*text*/ QColor("#333"),
                  /*bright_text*/ QColor("#f2f5f8"),
                  /*base*/ QColor("#fff"),
                  /*window*/ QColor("white"));
                lightActive.setColor(QPalette::AlternateBase, QColor("#eee"));
                lightActive.setColor(QPalette::Highlight, QColor("#38a3d8"));
                lightActive.setColor(QPalette::HighlightedText, QColor("#f4f4f5"));
                lightActive.setColor(QPalette::ToolTipBase, lightActive.base().color());
                lightActive.setColor(QPalette::ToolTipText, lightActive.text().color());
                lightActive.setColor(QPalette::Link, QColor("#0077b5"));
                lightActive.setColor(QPalette::ButtonText, QColor("#555459"));
                return lightActive;
        } else if (theme == "dark") {
                QPalette darkActive(
                  /*windowText*/ QColor("#caccd1"),
                  /*button*/ QColor(0xff, 0xff, 0xff),
                  /*light*/ QColor("#caccd1"),
                  /*dark*/ QColor(60, 70, 77),
                  /*mid*/ QColor("#202228"),
                  /*text*/ QColor("#caccd1"),
                  /*bright_text*/ QColor("#f4f5f8"),
                  /*base*/ QColor("#202228"),
                  /*window*/ QColor("#2d3139"));
                darkActive.setColor(QPalette::AlternateBase, QColor("#2d3139"));
                darkActive.setColor(QPalette::Highlight, QColor("#38a3d8"));
                darkActive.setColor(QPalette::HighlightedText, QColor("#f4f5f8"));
                darkActive.setColor(QPalette::ToolTipBase, darkActive.base().color());
                darkActive.setColor(QPalette::ToolTipText, darkActive.text().color());
                darkActive.setColor(QPalette::Link, QColor("#38a3d8"));
                darkActive.setColor(QPalette::ButtonText, "#727274");
                return darkActive;
        } else {
                return original;
        }
}

Theme::Theme(std::string_view theme)
{
        auto p     = paletteFromTheme(theme);
        separator_ = p.mid().color();
        if (theme == "light") {
                sidebarBackground_ = QColor("#233649");
                alternateButton_   = QColor("#ccc");
                red_               = QColor("#a82353");
        } else if (theme == "dark") {
                sidebarBackground_ = QColor("#2d3139");
                alternateButton_   = QColor("#414A59");
                red_               = QColor("#a82353");
        } else {
                sidebarBackground_ = p.window().color();
                alternateButton_   = p.dark().color();
                red_               = QColor("red");
        }
}
