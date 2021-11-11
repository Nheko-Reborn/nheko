// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import "../ui/media"
import QtMultimedia 5.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
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

        property double tempWidth: Math.min(parent ? parent.width : undefined, originalWidth < 1 ? 400 : originalWidth)
        property double tempHeight: tempWidth * proportionalHeight
        property double divisor: isReply ? 4 : 2
        property bool tooHigh: tempHeight > timelineRoot.height / divisor

        color: type == MtxEvent.VideoMessage ? Nheko.colors.window : "transparent"
        Layout.preferredHeight: type == MtxEvent.VideoMessage ? tooHigh ? timelineRoot.height / divisor : tempHeight : 80
        Layout.preferredWidth: type == MtxEvent.VideoMessage ? tooHigh ? (timelineRoot.height / divisor) / proportionalHeight : tempWidth : 250

        Image {
            anchors.fill: parent
            source: thumbnailUrl.replace("mxc://", "image://MxcImage/")
            asynchronous: true
            fillMode: Image.PreserveAspectFit

            VideoOutput {
                id: videoOutput

                visible: type == MtxEvent.VideoMessage
                clip: true
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
                source: mxcmedia
                flushMode: VideoOutput.FirstFrame
            }

            MediaControls {
                id: mediaControls

                anchors.fill: parent
                x: type == MtxEvent.VideoMessage ? videoOutput.contentRect.x : videoContainer.x
                y: type == MtxEvent.VideoMessage ? videoOutput.contentRect.y : videoContainer.y
                width: type == MtxEvent.VideoMessage ? videoOutput.contentRect.width : videoContainer.width
                height: type == MtxEvent.VideoMessage ? videoOutput.contentRect.height : videoContainer.height
                playingVideo: type == MtxEvent.VideoMessage
                positionValue: mxcmedia.position
                duration: mxcmedia.duration
                mediaLoaded: mxcmedia.loaded
                mediaState: mxcmedia.state
                onPositionChanged: mxcmedia.position = position
                onPlayPauseActivated: mxcmedia.state == MediaPlayer.PlayingState ? mxcmedia.pause() : mxcmedia.play()
                onLoadActivated: mxcmedia.eventId = eventId
            }

        }

    }

    // information about file name and file size
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
