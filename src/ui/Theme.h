// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QPalette>

namespace ui {
enum class AvatarType
{
        Image,
        Letter
};

// Default font size.
const int FontSize = 16;

// Default avatar size. Width and height.
const int AvatarSize = 40;

enum class ButtonPreset
{
        FlatPreset,
        CheckablePreset
};

enum class RippleStyle
{
        CenteredRipple,
        PositionedRipple,
        NoRipple
};

enum class OverlayStyle
{
        NoOverlay,
        TintedOverlay,
        GrayOverlay
};

enum class Role
{
        Default,
        Primary,
        Secondary
};

enum class ButtonIconPlacement
{
        LeftIcon,
        RightIcon
};

enum class ProgressType
{
        DeterminateProgress,
        IndeterminateProgress
};

} // namespace ui

class Theme : public QPalette
{
        Q_GADGET
        Q_PROPERTY(QColor sidebarBackground READ sidebarBackground CONSTANT)
        Q_PROPERTY(QColor separator READ separator CONSTANT)
public:
        Theme() {}
        explicit Theme(std::string_view theme);
        static QPalette paletteFromTheme(std::string_view theme);

        QColor sidebarBackground() const { return sidebarBackground_; }
        QColor separator() const { return separator_; }

private:
        QColor sidebarBackground_, separator_;
};
