// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import "../ui/media"
import QtMultimedia
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko

Item {
    id: content

    required property string body
    property double divisor: isReply ? 4 : 2
    required property int duration
    required property string eventId
    required property string filesize
    property bool fitsMetadata: (parent.width - fileInfoLabel.width) > metadataWidth + 4
    property int metadataWidth
    required property int originalWidth
    required property double proportionalHeight
    property int tempWidth: originalWidth < 1 ? 400 : originalWidth
    required property string thumbnailUrl
    required property int type
    required property string url

    height: (type == MtxEvent.VideoMessage ? width * proportionalHeight : 80) + fileInfoLabel.height
    implicitHeight: height
    implicitWidth: type == MtxEvent.VideoMessage ? Math.round(tempWidth * Math.min((timelineView.height / divisor) / (tempWidth * proportionalHeight), 1)) : 500
    width: Math.min(parent.width, implicitWidth)

    MxcMedia {
        id: mxcmedia
        roomm: room
        videoOutput: videoOutput

        audioOutput: AudioOutput {
            muted: mediaControls.muted
            volume: mediaControls.desiredVolume
        }
    }
    Rectangle {
        id: videoContainer
        color: type == MtxEvent.VideoMessage ? timelineRoot.palette.window : "transparent"
        height: parent.height - fileInfoLabel.height
        width: parent.width

        TapHandler {
            onTapped: Settings.openVideoExternal ? room.openMedia(eventId) : mediaControls.showControls()
        }
        Image {
            anchors.fill: parent
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            source: thumbnailUrl ? thumbnailUrl.replace("mxc://", "image://MxcImage/") + "?scale" : ""

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                clip: true
                fillMode: VideoOutput.PreserveAspectFit
                //flushMode: VideoOutput.FirstFrame
                orientation: mxcmedia.orientation
                visible: type == MtxEvent.VideoMessage
            }
        }
    }
    MediaControls {
        id: mediaControls
        anchors.bottom: fileInfoLabel.top
        anchors.left: content.left
        anchors.right: content.right
        duration: mediaLoaded ? mxcmedia.duration : content.duration
        mediaLoaded: mxcmedia.loaded
        mediaState: mxcmedia.state
        playingVideo: type == MtxEvent.VideoMessage
        positionValue: mxcmedia.position

        onLoadActivated: mxcmedia.eventId = eventId
        onPlayPauseActivated: mxcmedia.state == MediaPlayer.PlayingState ? mxcmedia.pause() : mxcmedia.play()
        onPositionChanged: mxcmedia.position = position
    }

    // information about file name and file size
    Label {
        id: fileInfoLabel
        anchors.bottom: content.bottom
        color: timelineRoot.palette.text
        elide: Text.ElideRight
        text: body + " [" + filesize + "]"
        textFormat: Text.RichText

        background: Rectangle {
            color: timelineRoot.palette.base
        }
    }
}
