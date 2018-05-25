#pragma once

#include <QRegExp>
#include <QString>

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

namespace strings {
const QString url_html = "<a href=\"\\1\">\\1</a>";
const QRegExp url_regex(
  "((www\\.(?!\\.)|[a-z][a-z0-9+.-]*://)[^\\s<>'\"]+[^!,\\.\\s<>'\"\\]\\)\\:])");
}

// Window geometry.
namespace window {
constexpr int height = 600;
constexpr int width  = 1066;

constexpr int minHeight = height;
constexpr int minWidth  = 950;
} // namespace window

namespace textInput {
constexpr int height = 50;
}

namespace sidebarActions {
constexpr int height   = textInput::height;
constexpr int iconSize = 28;
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
constexpr int dateSeparator = conf::fontSize;
} // namespace fonts
} // namespace timeline

} // namespace conf
