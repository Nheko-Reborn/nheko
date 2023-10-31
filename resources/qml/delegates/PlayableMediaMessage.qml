// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../ui/media"
import QtMultimedia
import QtQuick
import QtQuick.Controls
import im.nheko

Item {
    id: content

    required property string body
    property double divisor: EventDelegateChooser.isReply ? 10 : 4
    required property int duration
    required property string eventId
    required property string filesize
    property bool fitsMetadata: (parent.width - fileInfoLabel.width) > metadataWidth + 4
    //implicitHeight: height

    property int metadataWidth
    required property int originalWidth
    required property double proportionalHeight
    property int tempWidth: originalWidth < 1 ? 400 : originalWidth
    required property string thumbnailUrl
    required property int type
    required property string url

    height: (type == MtxEvent.VideoMessage ? width * proportionalHeight : 80) + fileInfoLabel.height
    implicitWidth: type == MtxEvent.VideoMessage ? Math.round(tempWidth * Math.min((timelineView.height / divisor) / (tempWidth * proportionalHeight), 1)) : 500
    width: Math.min(parent?.width ?? implicitWidth, implicitWidth)

    MxcMedia {
        id: mxcmedia

        // TODO: Show error in overlay or so?
        roomm: room
        videoOutput: videoOutput

        audioOutput: AudioOutput {
            muted: mediaControls.muted
            volume: mediaControls.desiredVolume
        }
    }
    Rectangle {
        id: videoContainer

        color: content.type == MtxEvent.VideoMessage ? palette.window : "transparent"
        height: parent.height - fileInfoLabel.height
        width: parent.width

        TapHandler {
            onTapped: Settings.openVideoExternal ? room.openMedia(eventId) : mediaControls.showControls()
        }
        Image {
            anchors.fill: parent
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            source: content.thumbnailUrl ? thumbnailUrl.replace("mxc://", "image://MxcImage/") + "?scale" : "image://colorimage/:/icons/icons/ui/video-file.svg?" + palette.windowText

            VideoOutput {
                id: videoOutput

                anchors.fill: parent
                clip: true
                fillMode: VideoOutput.PreserveAspectFit
                orientation: mxcmedia.orientation
                visible: content.type == MtxEvent.VideoMessage
            }
        }
        MediaControls {
            id: mediaControls

            anchors.bottom: videoContainer.bottom
            anchors.left: videoContainer.left
            anchors.right: videoContainer.right
            duration: mediaLoaded ? mxcmedia.duration : content.duration
            mediaLoaded: mxcmedia.loaded
            mediaState: mxcmedia.playbackState
            playingVideo: content.type == MtxEvent.VideoMessage
            positionValue: mxcmedia.position

            onLoadActivated: mxcmedia.eventId = eventId
            onPlayPauseActivated: mxcmedia.playbackState == MediaPlayer.PlayingState ? mxcmedia.pause() : mxcmedia.play()
            onPositionChanged: mxcmedia.position = position
        }
    }

    // information about file name and file size
    Label {
        id: fileInfoLabel

        anchors.top: videoContainer.bottom
        color: palette.text
        elide: Text.ElideRight
        text: content.body + " [" + filesize + "]"
        textFormat: Text.RichText

        background: Rectangle {
            color: palette.base
        }
    }
}
