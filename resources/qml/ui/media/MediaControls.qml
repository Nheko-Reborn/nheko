// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import "../../"
import QtMultimedia
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

Rectangle {
    id: control

    property alias desiredVolume: volumeSlider.desiredVolume
    property var duration
    property bool mediaLoaded: false
    property var mediaState
    property bool muted: false
    property bool playingVideo: false
    property var position
    property var positionValue: 0
    property bool shouldShowControls: !playingVideo || playerMouseArea.shouldShowControls || volumeSlider.state == "shown"

    signal loadActivated
    signal playPauseActivated

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
    function showControls() {
        controlHideTimer.restart();
    }

    color: {
        var wc = palette.alternateBase;
        return Qt.rgba(wc.r, wc.g, wc.b, 0.5);
    }
    height: controlLayout.implicitHeight
    opacity: control.shouldShowControls ? 1 : 0

    // Fade controls in/out
    Behavior on opacity {
        OpacityAnimator {
            duration: 100
        }
    }

    HoverHandler {
        id: playerMouseArea

        property bool shouldShowControls: hovered || controlHideTimer.running || control.mediaState != MediaPlayer.PlayingState

        onHoveredChanged: showControls()
    }
    ColumnLayout {
        id: controlLayout

        anchors.bottom: control.bottom
        anchors.left: control.left
        anchors.right: control.right
        enabled: control.shouldShowControls
        spacing: 0

        NhekoSlider {
            Layout.fillWidth: true
            Layout.leftMargin: Nheko.paddingSmall
            Layout.rightMargin: Nheko.paddingSmall
            alwaysShowSlider: false
            enabled: control.mediaLoaded
            from: 0
            to: control.duration
            value: control.positionValue

            onMoved: control.position = value
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Nheko.paddingSmall
            spacing: Nheko.paddingSmall

            // Cache/Play/pause button
            ImageButton {
                id: playbackStateImage

                Layout.alignment: Qt.AlignLeft
                Layout.preferredHeight: 24
                Layout.preferredWidth: 24
                buttonTextColor: palette.text
                image: {
                    if (control.mediaLoaded) {
                        if (control.mediaState == MediaPlayer.PlayingState)
                            return ":/icons/icons/ui/pause-symbol.svg";
                        else
                            return ":/icons/icons/ui/play-sign.svg";
                    } else {
                        return ":/icons/icons/ui/download.svg";
                    }
                }

                onClicked: control.mediaLoaded ? control.playPauseActivated() : control.loadActivated()
            }
            ImageButton {
                id: volumeButton

                Layout.alignment: Qt.AlignLeft
                Layout.preferredHeight: 24
                Layout.preferredWidth: 24
                buttonTextColor: palette.text
                image: {
                    if (control.muted || control.desiredVolume <= 0)
                        return ":/icons/icons/ui/volume-off-indicator.svg";
                    else
                        return ":/icons/icons/ui/volume-up.svg";
                }

                onClicked: control.muted = !control.muted
            }
            NhekoSlider {
                id: volumeSlider

                property real desiredVolume: volumeSlider.value

                Layout.alignment: Qt.AlignLeft
                Layout.preferredWidth: 0
                opacity: 0
                orientation: Qt.Horizontal
                state: ""
                value: 1

                states: State {
                    name: "shown"
                    when: Settings.mobileMode || volumeButton.hovered || volumeSlider.hovered || volumeSlider.pressed

                    PropertyChanges {
                        volumeSlider.implicitWidth: 100
                    }
                    PropertyChanges {
                        volumeSlider.opacity: 1
                    }
                }
                transitions: [
                    Transition {
                        from: ""
                        to: "shown"

                        SequentialAnimation {
                            PauseAnimation {
                                duration: 50
                            }
                            NumberAnimation {
                                duration: 100
                                easing.type: Easing.InQuad
                                properties: "opacity"
                            }
                        }
                        NumberAnimation {
                            duration: 150
                            properties: "Layout.preferredWidth"
                        }
                    },
                    Transition {
                        from: "shown"
                        to: ""

                        SequentialAnimation {
                            PauseAnimation {
                                duration: 100
                            }
                            ParallelAnimation {
                                NumberAnimation {
                                    duration: 100
                                    easing.type: Easing.InQuad
                                    properties: "opacity"
                                }
                                NumberAnimation {
                                    duration: 150
                                    properties: "Layout.preferredWidth"
                                }
                            }
                        }
                    }
                ]

                onDesiredVolumeChanged: {
                    control.muted = !(desiredVolume > 0);
                }
            }
            Label {
                Layout.alignment: Qt.AlignRight
                color: palette.text
                text: (!control.mediaLoaded ? "-- " : durationToString(control.positionValue)) + " / " + durationToString(control.duration)
            }
            Item {
                Layout.fillWidth: true
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
