#pragma once

// Non-theme app configuration. Layouts, fonts spacing etc.
//
// Font sizes are in pixels.

namespace conf
{
// Global settings.
static const int fontSize = 12;
static const int emojiSize = 14;
static const int headerFontSize = 21;

// Button settings.
namespace btn
{
static const int fontSize = 20;
static const int cornerRadius = 3;
}

// RoomList specific.
namespace roomlist
{
namespace fonts
{
static const int heading = 13;
static const int badge = 10;
static const int bubble = 20;
}  // namespace fonts
}  // namespace roomlist

namespace userInfoWidget
{
namespace fonts
{
static const int displayName = 16;
static const int userid = 14;
}  // namespace fonts
}  // namespace userInfoWidget

namespace topRoomBar
{
namespace fonts
{
static const int roomName = 15;
static const int roomDescription = 13;
}  // namespace fonts
}  // namespace topRoomBar

namespace timeline
{
static const int msgMargin = 11;
static const int avatarSize = 36;
static const int headerSpacing = 5;
static const int headerLeftMargin = 12;

namespace fonts
{
static const int timestamp = 9;
}
}

}  // namespace conf
