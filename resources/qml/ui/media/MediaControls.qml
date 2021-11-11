// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtMultimedia 5.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

Item {
    id: control

    property alias desiredVolume: volumeSlider.desiredVolume
    property alias muted: volumeSlider.muted
    property bool playingVideo: false
    property var mediaState
    property bool mediaLoaded: false
    property var duration
    property var positionValue: 0
    property var position
    property int controlHeight: 25
    property bool shouldShowControls: !playingVideo || playerMouseArea.shouldShowControls || volumeSlider.controlsVisible

    signal playPauseActivated(real mouseX, real mouseY)
    signal loadActivated(real mouseX, real mouseY)

    function durationToString(duration) {
        function maybeZeroPrepend(time) {
            return (time < 10) ? "0" + time.toString() : time.toString();
        }

        var totalSeconds = Math.floor(duration / 1000);
        var seconds = totalSeconds % 60;
        var minutes = (Math.floor(totalSeconds / 60)) % 60;
        var hours = (Math.floor(totalSeconds / (60 * 24))) % 24;
        // Always show minutes and don't prepend zero into the leftmost element
        var ss = maybeZeroPrepend(seconds);
        var mm = (hours > 0) ? maybeZeroPrepend(minutes) : minutes.toString();
        var hh = hours.toString();
        if (hours < 1)
            return mm + ":" + ss;

        return hh + ":" + mm + ":" + ss;
    }

    MouseArea {
        id: playerMouseArea

        property bool shouldShowControls: (containsMouse && controlHideTimer.running) || (control.mediaState != MediaPlayer.PlayingState) || controlLayout.contains(mapToItem(controlLayout, mouseX, mouseY))

        onClicked: {
            control.mediaLoaded ? control.playPauseActivated(mouseX, mouseY) : control.loadActivated(mouseX, mouseY);
        }
        hoverEnabled: true
        onPositionChanged: controlHideTimer.start()
        onExited: controlHideTimer.start()
        onEntered: controlHideTimer.start()
        anchors.fill: control
        propagateComposedEvents: true
    }

    ColumnLayout {

        id: controlLayout
        opacity: control.shouldShowControls ? 1 : 0

        // spacing: Nheko.paddingSmall
        anchors.bottom: control.bottom
        anchors.left: control.left
        anchors.right: control.right

        NhekoSlider {
            Layout.fillWidth: true
            Layout.minimumWidth: 50
            Layout.leftMargin: Nheko.paddingMedium
            Layout.rightMargin: Nheko.paddingMedium
            height: control.controlHeight
            value: control.positionValue
            onMoved: control.position = value
            from: 0
            to: control.duration
            sliderHeight: 8
            alwaysShowSlider: false
        }

        Rectangle {
            id: controlRect

            // Window color with 128/255 alpha
            color: {
                var wc = Nheko.colors.alternateBase;
                return Qt.rgba(wc.r, wc.g, wc.b, 0.5);
            }

            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom

            height: 35
            Layout.fillWidth: true

            RowLayout {
                anchors.left: controlRect.left
                anchors.bottom: controlRect.bottom
                anchors.right: controlRect.right
                anchors.margins: Nheko.paddingSmall
                anchors.verticalCenter: controlRect.verticalCenter
                spacing: Nheko.paddingSmall

                // Cache/Play/pause button
                Image {
                    Layout.alignment: Qt.AlignLeft
                    id: playbackStateImage

                    property color controlColor: (playbackStateArea.containsMouse) ? Nheko.colors.highlight : Nheko.colors.text

                    fillMode: Image.PreserveAspectFit
                    Layout.preferredHeight: control.controlHeight
                    source: {
                        if (control.mediaLoaded) {
                            if (control.mediaState == MediaPlayer.PlayingState)
                                return "image://colorimage/:/icons/icons/ui/pause-symbol.png?" + controlColor;
                            else
                                return "image://colorimage/:/icons/icons/ui/play-sign.png?" + controlColor;
                        } else {
                            return "image://colorimage/:/icons/icons/ui/arrow-pointing-down.png?" + controlColor;
                        }
                    }

                    MouseArea {
                        id: playbackStateArea

                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            control.mediaLoaded ? control.playPauseActivated(mouseX, mouseY) : control.loadActivated(mouseX, mouseY);
                        }
                    }

                }

                VolumeControl {
                    Layout.alignment: Qt.AlignLeft
                    id: volumeSlider
                    orientation: Qt.Horizontal
                    Layout.rightMargin: 5
                    Layout.preferredHeight: control.controlHeight
                }

                Label {
                    Layout.alignment: Qt.AlignRight

                    text: (!control.mediaLoaded) ? "-/-" : (durationToString(control.positionValue) + "/" + durationToString(control.duration))
                    color: Nheko.colors.text
                }

                Item {
                    Layout.fillWidth: true
                }

            }

        }

        // Fade controls in/out
        Behavior on opacity {
            OpacityAnimator {
                duration: 100
            }

        }

    }

    // For hiding controls on stationary cursor
    Timer {
        id: controlHideTimer

        interval: 1500 //ms
        repeat: false
    }

}
