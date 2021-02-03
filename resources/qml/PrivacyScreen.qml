import QtGraphicalEffects 1.0
import QtQuick 2.12
import im.nheko 1.0

Item {
    id: privacyScreen

    property var timelineRoot
    property int screenTimeout

    Connections {
        target: TimelineManager
        onFocusChanged: {
            if (TimelineManager.isWindowFocused) {
                screenSaverTimer.stop();
                screenSaver.state = "Invisible";
            } else {
                if (timelineRoot.visible)
                    screenSaverTimer.start();

            }
        }
    }

    Timer {
        id: screenSaverTimer

        interval: screenTimeout * 1000
        running: true
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
                from: "Visible"
                to: "Invisible"

                SequentialAnimation {
                    NumberAnimation {
                        target: screenSaver
                        property: "opacity"
                        duration: 250
                        easing.type: Easing.InQuad
                    }

                    NumberAnimation {
                        target: screenSaver
                        property: "visible"
                        duration: 0
                    }

                }

            },
            Transition {
                from: "Invisible"
                to: "Visible"

                SequentialAnimation {
                    NumberAnimation {
                        target: screenSaver
                        property: "visible"
                        duration: 0
                    }

                    NumberAnimation {
                        target: screenSaver
                        property: "opacity"
                        duration: 500
                        easing.type: Easing.InQuad
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
