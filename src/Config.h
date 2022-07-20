// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QRegularExpression>
#include <QString>

// clazy:excludeall=non-pod-global-static

// Non-theme app configuration. Layouts, fonts spacing etc.
//
// Font sizes are in pixels.
namespace conf {

namespace modals {
constexpr int WIDGET_MARGIN  = 20;
constexpr int WIDGET_SPACING = 15;

constexpr auto LABEL_MEDIUM_SIZE_RATIO = 1.3;
}

namespace strings {
const QString url_html = QStringLiteral("<a href=\"\\1\">\\1</a>");
const QRegularExpression url_regex(
  // match an URL, that is not quoted, i.e.
  // vvvvvv match quote via negative lookahead/lookbehind                              vv
  //          vvvv atomic match url -> fail if there is a " before or after        vvv
  QStringLiteral(
    R"((?<!["'])(?>((www\.(?!\.)|[a-z][a-z0-9+.-]*://)[^\s<>'"]+[^!,\.\s<>'"\]\)\:]))(?!["']))"));
// A matrix link to be converted back to markdown
static const QRegularExpression
  matrixToLink(QStringLiteral(R"(<a href=\"(https://matrix.to/#/.*?)\">(.*?)</a>)"));
}

// Window geometry.
namespace window {
constexpr int height = 600;
constexpr int width  = 1066;

constexpr int minHeight = 340;
constexpr int minWidth  = 340;
} // namespace window

} // namespace conf
