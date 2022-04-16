// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.12

SequentialAnimation {
    property alias glowDuration: numberAnimation.duration
    property double offset: 0
    property int pauseDuration: 150
    property alias target: numberAnimation.target

    loops: Animation.Infinite

    PauseAnimation {
        duration: pauseDuration * offset
    }
    NumberAnimation {
        id: numberAnimation
        from: 0
        property: "opacity"
        to: 1
    }
    PauseAnimation {
        duration: pauseDuration * (1 - offset)
    }
}
