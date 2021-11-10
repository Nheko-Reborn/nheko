// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import "../ui/media"
import QtMultimedia 5.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import im.nheko 1.0

ColumnLayout {
    id: content

    required property double proportionalHeight
    required property int type
    required property int originalWidth
    required property string thumbnailUrl
    required property string eventId
    required property string url
    required property string body
    required property string filesize

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

    Layout.fillWidth: true

    MxcMedia {
        id: mxcmedia

        // TODO: Show error in overlay or so?
        onError: console.log(error)
        roomm: room
        // desiredVolume is a float from 0.0 -> 1.0, MediaPlayer volume is an int from 0 to 100
        // this value automatically gets clamped for us between these two values.
        volume: mediaControls.desiredVolume * 100
        muted: mediaControls.muted
    }

    Rectangle {
        id: videoContainer

        //property double tempWidth: Math.min(parent ? parent.width : undefined, model.data.width < 1 ? 400 : /////model.data.width)
        // property double tempWidth: (model.data.width < 1) ? 400 : model.data.width
        // property double tempHeight: tempWidth * model.data.proportionalHeight
        //property double tempWidth: Math.min(parent ? parent.width : undefined, originalWidth < 1 ? 400 : originalWidth)
        property double tempWidth: Math.min(parent ? parent.width : undefined, originalWidth < 1 ? 400 : originalWidth)
        property double tempHeight: tempWidth * proportionalHeight
        property double divisor: isReply ? 4 : 2
        property bool tooHigh: tempHeight > timelineRoot.height / divisor

        visible: type == MtxEvent.VideoMessage
        color: Nheko.colors.window
        Layout.preferredHeight: tooHigh ? timelineRoot.height / divisor : tempHeight
        Layout.preferredWidth: tooHigh ? (timelineRoot.height / divisor) / proportionalHeight : tempWidth
        Layout.maximumWidth: Layout.preferredWidth

        Image {
            anchors.fill: parent
            source: thumbnailUrl.replace("mxc://", "image://MxcImage/")
            asynchronous: true
            fillMode: Image.PreserveAspectFit

            // Button and window colored overlay to cache media
            Item {
                // Display over video controls
                z: videoOutput.z + 1
                visible: !mxcmedia.loaded
                anchors.fill: parent

                //color: Nheko.colors.window
                //opacity: 0.5
                Image {
                    property color buttonColor: (cacheVideoArea.containsMouse) ? Nheko.colors.highlight : Nheko.colors.text

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: "image://colorimage/:/icons/icons/ui/arrow-pointing-down.png?" + buttonColor
                }

                MouseArea {
                    id: cacheVideoArea

                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: !mxcmedia.loaded
                    onClicked: mxcmedia.eventId = eventId
                }

            }

            VideoOutput {
                id: videoOutput

                clip: true
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
                source: mxcmedia
                flushMode: VideoOutput.FirstFrame

                MediaControls {
                    id: mediaControls

                    anchors.fill: parent
                    x: videoOutput.contentRect.x
                    y: videoOutput.contentRect.y
                    width: videoOutput.contentRect.width
                    height: videoOutput.contentRect.height
                    positionValue: mxcmedia.position
                    duration: mxcmedia.duration
                    mediaLoaded: mxcmedia.loaded
                    mediaState: mxcmedia.state
                    volumeOrientation: Qt.Vertical
                    onPositionChanged: mxcmedia.position = position
                    onActivated: mxcmedia.state == MediaPlayer.PlayingState ? mxcmedia.pause() : mxcmedia.play()
                }

            }

        }

    }
    // Audio player

    // TODO: share code with the video player
    Rectangle {
        id: audioControlRect

        property int controlHeight: 25

        visible: type != MtxEvent.VideoMessage
        Layout.preferredHeight: 40

        RowLayout {
            anchors.fill: parent
            width: parent.width

            // Play/pause button
            Image {
                id: audioPlaybackStateImage

                property color controlColor: (audioPlaybackStateArea.containsMouse) ? Nheko.colors.highlight : Nheko.colors.text

                fillMode: Image.PreserveAspectFit
                Layout.preferredHeight: controlRect.controlHeight
                Layout.alignment: Qt.AlignVCenter
                source: {
                    if (!mxcmedia.loaded)
                        return "image://colorimage/:/icons/icons/ui/arrow-pointing-down.png?" + controlColor;

                    return (mxcmedia.state == MediaPlayer.PlayingState) ? "image://colorimage/:/icons/icons/ui/pause-symbol.png?" + controlColor : "image://colorimage/:/icons/icons/ui/play-sign.png?" + controlColor;
                }

                MouseArea {
                    id: audioPlaybackStateArea

                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (!mxcmedia.loaded) {
                            mxcmedia.eventId = eventId;
                            return ;
                        }
                        (mxcmedia.state == MediaPlayer.PlayingState) ? mxcmedia.pause() : mxcmedia.play();
                    }
                }

            }

            Label {
                text: (!mxcmedia.loaded) ? "-/-" : durationToString(mxcmedia.position) + "/" + durationToString(mxcmedia.duration)
            }

            Slider {
                Layout.fillWidth: true
                Layout.minimumWidth: 50
                height: controlRect.controlHeight
                value: mxcmedia.position
                onMoved: mxcmedia.position = value
                from: 0
                to: mxcmedia.duration
            }

        }

    }

    Label {
        id: fileInfoLabel

        Layout.fillWidth: true
        text: body + " [" + filesize + "]"
        textFormat: Text.PlainText
        elide: Text.ElideRight
        color: Nheko.colors.text

        background: Rectangle {
            color: Nheko.colors.base
        }

    }

}
