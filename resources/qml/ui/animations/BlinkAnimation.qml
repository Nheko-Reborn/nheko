// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

SequentialAnimation {
    property alias target: numberAnimation.target
    property alias glowDuration: numberAnimation.duration
    property int pauseDuration: 150
    property double offset: 0

    loops: Animation.Infinite

    PauseAnimation {
        duration: pauseDuration * offset
    }

    NumberAnimation {
        id: numberAnimation

        property: "opacity"
        from: 0
        to: 1
    }

    PauseAnimation {
        duration: pauseDuration * (1 - offset)
    }

}
