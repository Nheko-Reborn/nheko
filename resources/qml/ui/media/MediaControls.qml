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
    property bool muted: false
    property bool playingVideo: false
    property var mediaState
    property bool mediaLoaded: false
    property var duration
    property var positionValue: 0
    property var position
    property bool shouldShowControls: !playingVideo || playerMouseArea.shouldShowControls || volumeSlider.state == "shown"

    signal playPauseActivated()
    signal loadActivated()

    function showControls() {
        controlHideTimer.restart();
    }

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

    color: {
        var wc = palette.alternateBase;
        return Qt.rgba(wc.r, wc.g, wc.b, 0.5);
    }
    opacity: control.shouldShowControls ? 1 : 0
    height: controlLayout.implicitHeight

    HoverHandler {
        id: playerMouseArea

        property bool shouldShowControls: hovered || controlHideTimer.running || control.mediaState != MediaPlayer.PlayingState

        onHoveredChanged: showControls()
    }

    ColumnLayout {
        id: controlLayout

        enabled: control.shouldShowControls
        spacing: 0
        anchors.bottom: control.bottom
        anchors.left: control.left
        width: Math.max(implicitWidth, control.width)

        NhekoSlider {
            Layout.fillWidth: true
            Layout.leftMargin: Nheko.paddingSmall
            Layout.rightMargin: Nheko.paddingSmall
            enabled: control.mediaLoaded
            value: control.positionValue
            onMoved: control.position = value
            from: 0
            to: control.duration
            alwaysShowSlider: false
        }

        RowLayout {
            Layout.margins: Nheko.paddingSmall
            spacing: Nheko.paddingSmall
            Layout.fillWidth: true

            // Cache/Play/pause button
            ImageButton {
                id: playbackStateImage

                Layout.alignment: Qt.AlignLeft
                buttonTextColor: palette.text
                Layout.preferredHeight: 24
                Layout.preferredWidth: 24
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
                buttonTextColor: palette.text
                Layout.preferredHeight: 24
                Layout.preferredWidth: 24
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

                state: ""
                Layout.alignment: Qt.AlignLeft
                Layout.preferredWidth: 100
                opacity: 0
                enabled: false
                orientation: Qt.Horizontal
                value: 1
                onDesiredVolumeChanged: {
                    control.muted = !(desiredVolume > 0);
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
                                properties: "opacity"
                                easing.type: Easing.InQuad
                            }

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
                                    properties: "opacity"
                                    easing.type: Easing.InQuad
                                }


                            }

                        }

                    }
                ]

                states: State {
                    name: "shown"
                    when: Settings.mobileMode || volumeButton.hovered || volumeSlider.hovered || volumeSlider.pressed

                    PropertyChanges {
                        volumeSlider.opacity: 1
                        volumeSlider.enabled: true
                    }

                }

            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                Layout.alignment: Qt.AlignRight
                text: (!control.mediaLoaded ? "-- " : durationToString(control.positionValue)) + " / " + durationToString(control.duration)
                color: palette.text
            }

        }

    }

    // For hiding controls on stationary cursor
    Timer {
        id: controlHideTimer

        interval: 1500 //ms
        repeat: false
    }

    // Fade controls in/out
    Behavior on opacity {
        OpacityAnimator {
            duration: 100
        }

    }

}
