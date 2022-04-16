// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

//import QtGraphicalEffects 1.0
import QtQuick 2.12
import QtQuick.Window 2.2
import im.nheko

Item {
    id: privacyScreen

    property int screenTimeout
    property var timelineRoot

    Connections {
        function onFocusChanged() {
            if (MainWindow.active) {
                screenSaverTimer.stop();
                screenSaver.state = "Invisible";
            } else {
                if (timelineRoot.visible)
                    screenSaverTimer.start();
            }
        }

        target: TimelineManager
    }
    Timer {
        id: screenSaverTimer
        interval: screenTimeout * 1000
        running: !MainWindow.active

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
                    target: screenSaver
                    visible: true
                }
                PropertyChanges {
                    opacity: 1
                    target: screenSaver
                }
            },
            State {
                name: "Invisible"

                PropertyChanges {
                    opacity: 0
                    target: screenSaver
                }
                PropertyChanges {
                    target: screenSaver
                    visible: false
                }
            }
        ]
        transitions: [
            Transition {
                from: "Visible"
                to: "Invisible"

                SequentialAnimation {
                    NumberAnimation {
                        duration: 250
                        easing.type: Easing.InQuad
                        property: "opacity"
                        target: screenSaver
                    }
                    NumberAnimation {
                        duration: 0
                        property: "visible"
                        target: screenSaver
                    }
                }
            },
            Transition {
                from: "Invisible"
                to: "Visible"

                SequentialAnimation {
                    NumberAnimation {
                        duration: 0
                        property: "visible"
                        target: screenSaver
                    }
                    NumberAnimation {
                        duration: 500
                        easing.type: Easing.InQuad
                        property: "opacity"
                        target: screenSaver
                    }
                }
            }
        ]

        //FastBlur {
        //    id: blur

        //    anchors.fill: parent
        //    source: timelineRoot
        //    radius: 50
        //}
    }
}
