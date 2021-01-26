import QtQuick 2.12
import QtGraphicalEffects 1.0

Item {
    property var timelineRoot
    property var imageSource
    property int screenTimeout
    anchors.fill: parent

    Timer {
        id: screenSaverTimer
        interval: screenTimeout * 1000
        running: true
        onTriggered: {
            timelineRoot.grabToImage(function(result) {
                imageSource = result.url;
                screenSaver.visible = true
                particles.resume()
            }, Qt.size(width, height))
        }
    }

    // Reset screensaver timer when clicks are received
    MouseArea {
        anchors.fill: parent
        // Pass mouse events through
        propagateComposedEvents: true
        hoverEnabled: true
        onClicked: {
            screenSaverTimer.restart();
            mouse.accepted = false;
        }
    }

    Rectangle {
        id: screenSaver
        anchors.fill: parent
        visible: false
        color: "transparent"

        Image {
            id: image
            visible : screenSaver.visible
            anchors.fill: parent
            source: imageSource
        }

        ShaderEffectSource {
            id: effectSource

            sourceItem: image
            anchors.fill: image
            sourceRect: Qt.rect(0,0, width, height)
        }

        FastBlur{
            id: blur
            anchors.fill: effectSource
            source: effectSource
            radius: 50
        }

        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: true
            hoverEnabled: true
            onClicked: { 
                screenSaver.visible = false;
                screenSaverTimer.restart();
                mouse.accepted = false
            }
        }
    }
}