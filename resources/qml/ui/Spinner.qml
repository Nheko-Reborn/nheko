// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtGraphicalEffects 1.12

Item {
    id: spinner

    property int spacing: 0
    property bool running: true
    property var foreground: "#333"
    readonly property int barCount: 6
    readonly property real a: Math.PI / 6
    readonly property var colors: ["#c0def5", "#87aade", "white"]
    readonly property var anims: [anim1, anim2, anim3, anim4, anim5, anim6]

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

        SequentialAnimation {
            id: anim1

            loops: Animation.Infinite

            NumberAnimation {
                target: rect1
                property: "opacity"
                from: 0
                to: 1
                duration: 300
            }

            PauseAnimation {
                duration: spinner.barCount * 150
            }

        }

        SequentialAnimation {
            id: anim2

            loops: Animation.Infinite

            NumberAnimation {
                target: rect2
                property: "opacity"
                from: 0
                to: 1
                duration: 300
            }

            PauseAnimation {
                duration: spinner.barCount * 150
            }

        }

        SequentialAnimation {
            id: anim3

            loops: Animation.Infinite

            NumberAnimation {
                target: rect3
                property: "opacity"
                from: 0
                to: 1
                duration: 300
            }

            PauseAnimation {
                duration: spinner.barCount * 150
            }

        }

        SequentialAnimation {
            id: anim4

            loops: Animation.Infinite

            NumberAnimation {
                target: rect4
                property: "opacity"
                from: 0
                to: 1
                duration: 300
            }

            PauseAnimation {
                duration: spinner.barCount * 150
            }

        }

        SequentialAnimation {
            id: anim5

            loops: Animation.Infinite

            NumberAnimation {
                target: rect5
                property: "opacity"
                from: 0
                to: 1
                duration: 300
            }

            PauseAnimation {
                duration: spinner.barCount * 150
            }

        }

        SequentialAnimation {
            id: anim6

            loops: Animation.Infinite

            NumberAnimation {
                target: rect6
                property: "opacity"
                from: 0
                to: 1
                duration: 300
            }

            PauseAnimation {
                duration: spinner.barCount * 150
            }

        }

        transform: Matrix4x4 {
            matrix: Qt.matrix4x4(Math.cos(spinner.a), -Math.sin(spinner.a), 0, 0, 0, Math.cos(spinner.a), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
        }

    }

    Timer {
        // ----- Private Properties ----- //
        property int _barIndex: 0

        interval: 80
        repeat: true
        running: spinner.running
        onTriggered: {
            if (_barIndex === spinner.barCount) {
                _barIndex = 0;
                stop();
            } else {
                anims[_barIndex].start();
                _barIndex++;
            }
        }
        Component.onCompleted: start()
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
