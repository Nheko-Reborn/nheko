import QtGraphicalEffects 1.0
import QtQuick 2.12
import im.nheko 1.0

Item {
    id: privacyScreen

    property var timelineRoot
    property var imageSource: ""
    property int screenTimeout

    anchors.fill: parent

    Connections {
        target: TimelineManager
        onFocusChanged: {
            if (TimelineManager.isWindowFocused) {
                screenSaverTimer.stop();
                screenSaver.state = "Invisible";
            } else {
                if (timelineRoot.visible) {
                    screenSaverTimer.start();
                }
            }
        }
    }

    Timer {
        id: screenSaverTimer

        interval: screenTimeout * 1000
        running: true
        onTriggered: {
            if (MainWindow.visible) {
                timelineRoot.grabToImage(function(result) {
                    screenSaver.state = "Visible";
                    imageSource = result.url;
                }, Qt.size(width, height));
            }
        }
    }

    Rectangle {
        id: screenSaver

        state: "Invisible"
        anchors.fill: parent
        visible: false
        color: "transparent"
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

        Image {
            id: image

            cache: false
            anchors.fill: parent
            source: imageSource
        }

        ShaderEffectSource {
            id: effectSource

            sourceItem: image
            anchors.fill: image
            sourceRect: Qt.rect(0, 0, width, height)
        }

        FastBlur {
            id: blur

            anchors.fill: effectSource
            source: effectSource
            radius: 50
        }

    }

}
