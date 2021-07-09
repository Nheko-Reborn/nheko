// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtGraphicalEffects 1.12

SequentialAnimation {
    property alias target: numberAnimation.target
    property alias glowDuration: numberAnimation.duration
    property alias pauseDuration: pauseAnimation.duration

    loops: Animation.Infinite

    NumberAnimation {
        id: numberAnimation
        property: "opacity"
        from: 0
        to: 1
    }

    PauseAnimation {
        id: pauseAnimation
    }

}