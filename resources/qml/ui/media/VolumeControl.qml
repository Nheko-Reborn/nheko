// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"

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
    width: sourceSize.width + volumeSliderRect.implicitWidth

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
        anchors.bottom: volumeSlider.orientation == Qt.Vertical ? volumeImage.top : undefined
        anchors.left: volumeSlider.orientation == Qt.Vertical ? undefined : volumeImage.right
        anchors.horizontalCenter: volumeSlider.orientation == Qt.Vertical ? volumeImage.horizontalCenter : undefined
        anchors.verticalCenter: volumeSlider.orientation == Qt.Vertical ? undefined : volumeImage.verticalCenter
        color: {
            if (volumeSlider.orientation == Qt.Vertical) {
                var wc = Nheko.colors.window;
                return Qt.rgba(wc.r, wc.g, wc.b, 0.5);
            } else {
                return "transparent";
            }
        }
        /* TODO: base width on the slider width (some issue with it not having a geometry
        when using the width here?) */
        width: volumeSlider.orientation == Qt.Vertical ? volumeImage.width * 0.7 : 100
        radius: volumeSlider.width / 2
        height: volumeSlider.orientation == Qt.Vertical ? 100 : volumeImage.height * 0.7
        visible: volumeImageArea.containsMouse || volumeSliderHideTimer.running || volumeSliderRectMouseArea.containsMouse

        NhekoSlider {
            // TODO: the slider is slightly off-center on the left for some reason...
            id: volumeSlider

            sliderWidth: 8
            sliderHeight: 8
            // Desired value to avoid loop onMoved -> media.volume -> value -> onMoved...
            property real desiredVolume: QtMultimedia.convertVolume(volumeSlider.value, QtMultimedia.LogarithmicVolumeScale, QtMultimedia.LinearVolumeScale)

            value: 1
            anchors.fill: volumeSliderRect
            anchors.horizontalCenter: orientation == Qt.Vertical ? volumeSliderRect.horizontalCenter : undefined
            anchors.verticalCenter: orientation == Qt.Vertical ? undefined : volumeSliderRect.verticalCenter
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

    }

}
