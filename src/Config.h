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
// Global settings.
constexpr int fontSize                   = 14;
constexpr int textInputFontSize          = 14;
constexpr int emojiSize                  = 14;
constexpr int headerFontSize             = 21;
constexpr int typingNotificationFontSize = 11;

namespace popup {
constexpr int font   = fontSize;
constexpr int avatar = 28;
}

namespace modals {
constexpr int errorFont = conf::fontSize - 2;
}

namespace receipts {
constexpr int font = 12;
}

namespace dialogs {
constexpr int labelSize = 15;
}

namespace modals {
constexpr int WIDGET_MARGIN     = 20;
constexpr int WIDGET_SPACING    = 15;
constexpr int WIDGET_TOP_MARGiN = 2 * WIDGET_MARGIN;

constexpr int TEXT_SPACING = 4;

constexpr int BUTTON_SIZE   = 36;
constexpr int BUTTON_RADIUS = BUTTON_SIZE / 2;

constexpr auto LABEL_MEDIUM_SIZE_RATIO = 1.3;
constexpr auto LABEL_BIG_SIZE_RATIO    = 2;
}

namespace strings {
const QString url_html = QStringLiteral("<a href=\"\\1\">\\1</a>");
const QRegularExpression url_regex(
  // match an URL, that is not quoted, i.e.
  // vvvvvv match quote via negative lookahead/lookbehind                              vv
  //          vvvv atomic match url -> fail if there is a " before or after        vvv
  QStringLiteral(
    R"((?<!["'])(?>((www\.(?!\.)|[a-z][a-z0-9+.-]*://)[^\s<>'"]+[^!,\.\s<>'"\]\)\:]))(?!["']))"));
// match any markdown matrix.to link. Capture group 1 is the link name, group 2 is the target.
static const QRegularExpression
  matrixToMarkdownLink(QStringLiteral(R"(\[(.*?)(?<!\\)\]\((https://matrix.to/#/.*?\)))"));
static const QRegularExpression
  matrixToLink(QStringLiteral(R"(<a href=\"(https://matrix.to/#/.*?)\">(.*?)</a>)"));
}

// Window geometry.
namespace window {
constexpr int height        = 600;
constexpr int width         = 1066;
constexpr int minModalWidth = 340;

constexpr int minHeight = 340;
constexpr int minWidth  = 340;
} // namespace window

namespace textInput {
constexpr int height = 44;
}

namespace sidebarActions {
constexpr int height   = textInput::height;
constexpr int iconSize = 24;
}

// Button settings.
namespace btn {
constexpr int fontSize     = 20;
constexpr int cornerRadius = 3;
} // namespace btn

// RoomList specific.
namespace roomlist {
namespace fonts {
constexpr int heading         = 13;
constexpr int timestamp       = heading;
constexpr int badge           = 10;
constexpr int bubble          = 20;
constexpr int communityBubble = bubble - 4;
} // namespace fonts
} // namespace roomlist

namespace userInfoWidget {
namespace fonts {
constexpr int displayName = 15;
constexpr int userid      = 13;
} // namespace fonts
} // namespace userInfoWidget

namespace topRoomBar {
namespace fonts {
constexpr int roomName        = 15;
constexpr int roomDescription = 14;
} // namespace fonts
} // namespace topRoomBar

namespace timeline {
constexpr int msgAvatarTopMargin = 15;
constexpr int msgTopMargin       = 2;
constexpr int msgLeftMargin      = 14;
constexpr int avatarSize         = 36;
constexpr int headerSpacing      = 3;
constexpr int headerLeftMargin   = 15;

namespace fonts {
constexpr int timestamp     = 13;
constexpr int indicator     = timestamp - 2;
constexpr int dateSeparator = conf::fontSize;
} // namespace fonts
} // namespace timeline

} // namespace conf
