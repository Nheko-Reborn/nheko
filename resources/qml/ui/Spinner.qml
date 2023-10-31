// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./animations"
import QtQuick
import QtQuick.Effects

Item {
    id: spinner

    readonly property real a: Math.PI / 6
    readonly property var anims: [anim1, anim2, anim3, anim4, anim5, anim6]
    readonly property int barCount: 6
    readonly property var colors: ["#c0def5", "#87aade", "white"]
    property var foreground: "#333"
    readonly property int glowDuration: 300
    readonly property int pauseDuration: barCount * 150
    property bool running: true
    property int spacing: 0

    height: 40
    width: barCount * (height * 0.375)

    Row {
        id: row

        transform: Matrix4x4 {
            matrix: Qt.matrix4x4(Math.cos(spinner.a), -Math.sin(spinner.a), 0, 0, 0, Math.cos(spinner.a), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
        }

        Rectangle {
            id: rect1

            color: "white"
            height: spinner.height / 3.5
            width: ((spinner.width / spinner.barCount) - (spinner.spacing)) * 1.5
        }
        Rectangle {
            id: rect2

            color: spinner.colors[0]
            height: spinner.height
            width: (spinner.width / spinner.barCount) - spinner.spacing
        }
        Rectangle {
            id: rect3

            color: spinner.colors[1]
            height: spinner.height
            width: (spinner.width / spinner.barCount) - spinner.spacing
        }
        Rectangle {
            id: rect4

            color: spinner.colors[2]
            height: spinner.height
            width: (spinner.width / spinner.barCount) - spinner.spacing
        }
        Rectangle {
            id: rect5

            color: "white"
            height: spinner.height / 3.5
            width: (spinner.width / (spinner.barCount + 1)) - spinner.spacing
        }
        Rectangle {
            id: rect6

            color: "white"
            height: spinner.height
            width: (spinner.width / spinner.barCount) - spinner.spacing
        }
        BlinkAnimation {
            id: anim1

            glowDuration: spinner.glowDuration
            offset: 0 / spinner.barCount
            pauseDuration: spinner.pauseDuration
            running: spinner.running
            target: rect1
        }
        BlinkAnimation {
            id: anim2

            glowDuration: spinner.glowDuration
            offset: 1 / spinner.barCount
            pauseDuration: spinner.pauseDuration
            running: spinner.running
            target: rect2
        }
        BlinkAnimation {
            id: anim3

            glowDuration: spinner.glowDuration
            offset: 2 / spinner.barCount
            pauseDuration: spinner.pauseDuration
            running: spinner.running
            target: rect3
        }
        BlinkAnimation {
            id: anim4

            glowDuration: spinner.glowDuration
            offset: 3 / spinner.barCount
            pauseDuration: spinner.pauseDuration
            running: spinner.running
            target: rect4
        }
        BlinkAnimation {
            id: anim5

            glowDuration: spinner.glowDuration
            offset: 4 / spinner.barCount
            pauseDuration: spinner.pauseDuration
            running: spinner.running
            target: rect5
        }
        BlinkAnimation {
            id: anim6

            glowDuration: spinner.glowDuration
            offset: 5 / spinner.barCount
            pauseDuration: spinner.pauseDuration
            running: spinner.running
            target: rect6
        }
    }
    MultiEffect {
        anchors.fill: row
        shadowBlur: 14
        shadowColor: spinner.foreground
        shadowEnabled: true
        source: row

        transform: Matrix4x4 {
            matrix: Qt.matrix4x4(Math.cos(spinner.a), -Math.sin(spinner.a), 0, 0, 0, Math.cos(spinner.a), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
        }
    }
}
