// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtMultimedia 5.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

// Volume slider activator
Image {
    // TODO: add icons for different volume levels
    id: volumeImage

    property alias desiredVolume: volumeSlider.desiredVolume
    property alias orientation: volumeSlider.orientation
    property alias controlsVisible: volumeSliderRect.visible
    property bool muted: false
    property color controlColor: (volumeImageArea.containsMouse) ? Nheko.colors.highlight : Nheko.colors.text

    source: (desiredVolume > 0 && !muted) ? "image://colorimage/:/icons/icons/ui/volume-up.png?" + controlColor : "image://colorimage/:/icons/icons/ui/volume-off-indicator.png?" + controlColor
    fillMode: Image.PreserveAspectFit

    MouseArea {
        id: volumeImageArea

        anchors.fill: parent
        hoverEnabled: true
        onExited: volumeSliderHideTimer.start()
        onPositionChanged: volumeSliderHideTimer.start()
        onClicked: volumeImage.muted = !volumeImage.muted

        // For hiding volume slider after a while
        Timer {
            id: volumeSliderHideTimer

            interval: 1500
            repeat: false
            running: false
        }

    }

    Rectangle {
        id: volumeSliderRect

        opacity: (visible) ? 1 : 0
        anchors.bottom: volumeImage.top
        anchors.bottomMargin: 10
        anchors.horizontalCenter: volumeImage.horizontalCenter
        color: {
            var wc = Nheko.colors.window;
            return Qt.rgba(wc.r, wc.g, wc.b, 0.5);
        }
        /* TODO: base width on the slider width (some issue with it not having a geometry
        when using the width here?) */
        width: volumeImage.width * 0.7
        radius: volumeSlider.width / 2
        height: controlRect.height * 2 //100
        visible: volumeImageArea.containsMouse || volumeSliderHideTimer.running || volumeSliderRectMouseArea.containsMouse

        Slider {
            // TODO: the slider is slightly off-center on the left for some reason...
            id: volumeSlider

            // Desired value to avoid loop onMoved -> media.volume -> value -> onMoved...
            property real desiredVolume: QtMultimedia.convertVolume(volumeSlider.value, QtMultimedia.LogarithmicVolumeScale, QtMultimedia.LinearVolumeScale)

            value: 1
            anchors.fill: volumeSliderRect
            anchors.bottomMargin: volumeSliderRect.height * 0.1
            anchors.topMargin: volumeSliderRect.height * 0.1
            anchors.horizontalCenter: volumeSliderRect.horizontalCenter
            orientation: Qt.Vertical
            onDesiredVolumeChanged: {
                volumeImage.muted = !(desiredVolume > 0);
            }
        }
        // Used for resetting the timer on mouse moves on volumeSliderRect

        MouseArea {
            id: volumeSliderRectMouseArea

            anchors.fill: parent
            hoverEnabled: true
            propagateComposedEvents: true
            onExited: volumeSliderHideTimer.start()
            onClicked: mouse.accepted = false
            onPressed: mouse.accepted = false
            onReleased: mouse.accepted = false
            onPressAndHold: mouse.accepted = false
            onPositionChanged: {
                mouse.accepted = false;
                volumeSliderHideTimer.start();
            }
        }

        Behavior on opacity {
            OpacityAnimator {
                duration: 100
            }

        }
        // TODO: figure out a better way to put the slider popup above controlRect

    }

}
