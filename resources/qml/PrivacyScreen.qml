// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window
import im.nheko
import QtQuick.Effects

Item {
    id: privacyScreen

    readonly property bool active: Settings.privacyScreen && screenSaver.state === "Visible"
    property int screenTimeout
    property var timelineRoot
    required property var windowTarget

    Connections {
        function onActiveChanged() {
            if (windowTarget.active) {
                screenSaverTimer.stop();
                screenSaver.state = "Invisible";
            } else {
                if (timelineRoot.visible)
                    screenSaverTimer.start();
            }
        }

        target: windowTarget
    }
    Timer {
        id: screenSaverTimer

        interval: screenTimeout * 1000
        running: !windowTarget.active

        onTriggered: {
            screenSaver.state = "Visible";
        }
    }
    Item {
        id: screenSaver

        anchors.fill: parent
        state: "Invisible"
        visible: false

        states: [
            State {
                name: "Visible"

                PropertyChanges {
                    screenSaver.visible: true
                    screenSaver.opacity: 1
                }
            },
            State {
                name: "Invisible"

                PropertyChanges {
                    screenSaver.opacity: 0
                    screenSaver.visible: false
                }
            }
        ]
        transitions: [
            Transition {
                from: "Invisible"
                reversible: true
                to: "Visible"

                SequentialAnimation {
                    NumberAnimation {
                        duration: 0
                        property: "visible"
                        target: screenSaver
                    }
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.Linear
                        property: "opacity"
                        target: screenSaver
                    }
                }
            }
        ]

        MultiEffect {
            id: blur

            anchors.fill: parent
            blur: 1.0
            blurEnabled: true
            blurMax: 32
            source: timelineRoot
        }
    }
}
