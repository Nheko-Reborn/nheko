// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtGraphicalEffects 1.0
import QtQuick 2.12
import QtQuick.Window 2.2
import im.nheko 1.0

Item {
    id: privacyScreen

    readonly property bool active: Settings.privacyScreen && screenSaver.state === "Visible"
    property var timelineRoot
    property int screenTimeout

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

        state: "Invisible"
        anchors.fill: parent
        visible: false
        states: [
            State {
                name: "Visible"

                PropertyChanges {
                    target: screenSaver
                    visible: true
                }

                PropertyChanges {
                    target: screenSaver
                    opacity: 1
                }
            },
            State {
                name: "Invisible"

                PropertyChanges {
                    target: screenSaver
                    opacity: 0
                }

                PropertyChanges {
                    target: screenSaver
                    visible: false
                }
            }
        ]
        transitions: [
            Transition {
                from: "Invisible"
                to: "Visible"
                reversible: true

                SequentialAnimation {
                    NumberAnimation {
                        target: screenSaver
                        property: "visible"
                        duration: 0
                    }

                    NumberAnimation {
                        target: screenSaver
                        property: "opacity"
                        duration: 300
                        easing.type: Easing.Linear
                    }

                }

            }
        ]

        FastBlur {
            id: blur

            anchors.fill: parent
            source: timelineRoot
            radius: 50
        }

    }

}
