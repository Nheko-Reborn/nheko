#pragma once

#include <QRegExp>
#include <QString>

// Non-theme app configuration. Layouts, fonts spacing etc.
//
// Font sizes are in pixels.

namespace conf {
// Global settings.
static constexpr int fontSize                   = 14;
static constexpr int textInputFontSize          = 14;
static constexpr int emojiSize                  = 14;
static constexpr int headerFontSize             = 21;
static constexpr int typingNotificationFontSize = 11;

namespace receipts {
static constexpr int font = 12;
}

namespace dialogs {
static constexpr int labelSize = 15;
}

namespace strings {
static const QString url_html = "<a href=\"\\1\">\\1</a>";
static const QRegExp url_regex(
  "((www\\.(?!\\.)|[a-z][a-z0-9+.-]*://)[^\\s<>'\"]+[^!,\\.\\s<>'\"\\]\\)\\:])");
}

// Window geometry.
namespace window {
static constexpr int height = 600;
static constexpr int width  = 1066;

static constexpr int minHeight = height;
static constexpr int minWidth  = 950;
} // namespace window

namespace textInput {
static constexpr int height = 50;
}

namespace sidebarActions {
static constexpr int height   = textInput::height;
static constexpr int iconSize = 28;
}

// Button settings.
namespace btn {
static constexpr int fontSize     = 20;
static constexpr int cornerRadius = 3;
} // namespace btn

// RoomList specific.
namespace roomlist {
namespace fonts {
static constexpr int heading   = 13;
static constexpr int timestamp = heading;
static constexpr int badge     = 10;
static constexpr int bubble    = 20;
} // namespace fonts
} // namespace roomlist

namespace userInfoWidget {
namespace fonts {
static constexpr int displayName = 16;
static constexpr int userid      = 14;
} // namespace fonts
} // namespace userInfoWidget

namespace topRoomBar {
namespace fonts {
static constexpr int roomName        = 15;
static constexpr int roomDescription = 14;
} // namespace fonts
} // namespace topRoomBar

namespace timeline {
static constexpr int msgMargin        = 14;
static constexpr int avatarSize       = 36;
static constexpr int headerSpacing    = 7;
static constexpr int headerLeftMargin = 15;

namespace fonts {
static constexpr int timestamp     = 13;
static constexpr int dateSeparator = conf::fontSize;
} // namespace fonts
} // namespace timeline

} // namespace conf
