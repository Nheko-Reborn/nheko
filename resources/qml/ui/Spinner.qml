// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./animations"
import QtGraphicalEffects 1.12
import QtQuick 2.12

Item {
    id: spinner

    property int spacing: 0
    property bool running: true
    property var foreground: "#333"
    readonly property int barCount: 6
    readonly property real a: Math.PI / 6
    readonly property var colors: ["#c0def5", "#87aade", "white"]
    readonly property var anims: [anim1, anim2, anim3, anim4, anim5, anim6]
    readonly property int pauseDuration: barCount * 150
    readonly property int glowDuration: 300

    height: 40
    width: barCount * (height * 0.375)

    Row {
        id: row

        Rectangle {
            id: rect1

            width: ((spinner.width / spinner.barCount) - (spinner.spacing)) * 1.5
            height: spinner.height / 3.5
            color: "white"
        }

        Rectangle {
            id: rect2

            width: (spinner.width / spinner.barCount) - spinner.spacing
            height: spinner.height
            color: spinner.colors[0]
        }

        Rectangle {
            id: rect3

            width: (spinner.width / spinner.barCount) - spinner.spacing
            height: spinner.height
            color: spinner.colors[1]
        }

        Rectangle {
            id: rect4

            width: (spinner.width / spinner.barCount) - spinner.spacing
            height: spinner.height
            color: spinner.colors[2]
        }

        Rectangle {
            id: rect5

            width: (spinner.width / (spinner.barCount + 1)) - spinner.spacing
            height: spinner.height / 3.5
            color: "white"
        }

        Rectangle {
            id: rect6

            width: (spinner.width / spinner.barCount) - spinner.spacing
            height: spinner.height
            color: "white"
        }

        BlinkAnimation {
            id: anim1

            target: rect1
            running: spinner.running
            pauseDuration: spinner.pauseDuration
            glowDuration: spinner.glowDuration
            offset: 0 / spinner.barCount
        }

        BlinkAnimation {
            id: anim2

            target: rect2
            running: spinner.running
            pauseDuration: spinner.pauseDuration
            glowDuration: spinner.glowDuration
            offset: 1 / spinner.barCount
        }

        BlinkAnimation {
            id: anim3

            target: rect3
            running: spinner.running
            pauseDuration: spinner.pauseDuration
            glowDuration: spinner.glowDuration
            offset: 2 / spinner.barCount
        }

        BlinkAnimation {
            id: anim4

            target: rect4
            running: spinner.running
            pauseDuration: spinner.pauseDuration
            glowDuration: spinner.glowDuration
            offset: 3 / spinner.barCount
        }

        BlinkAnimation {
            id: anim5

            target: rect5
            running: spinner.running
            pauseDuration: spinner.pauseDuration
            glowDuration: spinner.glowDuration
            offset: 4 / spinner.barCount
        }

        BlinkAnimation {
            id: anim6

            target: rect6
            running: spinner.running
            pauseDuration: spinner.pauseDuration
            glowDuration: spinner.glowDuration
            offset: 5 / spinner.barCount
        }

        transform: Matrix4x4 {
            matrix: Qt.matrix4x4(Math.cos(spinner.a), -Math.sin(spinner.a), 0, 0, 0, Math.cos(spinner.a), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
        }

    }

    Glow {
        anchors.fill: row
        radius: 14
        samples: 17
        color: spinner.foreground
        source: row

        transform: Matrix4x4 {
            matrix: Qt.matrix4x4(Math.cos(spinner.a), -Math.sin(spinner.a), 0, 0, 0, Math.cos(spinner.a), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
        }

    }

}
