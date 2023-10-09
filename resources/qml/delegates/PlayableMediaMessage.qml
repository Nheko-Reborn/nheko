// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import "../ui/media"
import QtMultimedia
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

Item {
    id: content

    required property double proportionalHeight
    required property int type
    required property int originalWidth
    required property int duration
    required property string thumbnailUrl
    required property string eventId
    required property string url
    required property string body
    required property string filesize
    property double divisor: EventDelegateChooser.isReply ? 10 : 4
    property int tempWidth: originalWidth < 1? 400: originalWidth
    implicitWidth: type == MtxEvent.VideoMessage ? Math.round(tempWidth*Math.min((timelineView.height/divisor)/(tempWidth*proportionalHeight), 1)) : 500
    width: Math.min(parent?.width ?? implicitWidth, implicitWidth)
    height: (type == MtxEvent.VideoMessage ? width*proportionalHeight : 80) + fileInfoLabel.height
    //implicitHeight: height

    property int metadataWidth
    property bool fitsMetadata: (parent.width - fileInfoLabel.width) > metadataWidth+4

    MxcMedia {
        id: mxcmedia

        // TODO: Show error in overlay or so?
        roomm: room
        audioOutput: AudioOutput {
            muted: mediaControls.muted
            volume: mediaControls.desiredVolume
        }
        videoOutput: videoOutput
    }

    Rectangle {
        id: videoContainer

        color: type == MtxEvent.VideoMessage ? palette.window : "transparent"
        width: parent.width
        height: parent.height - fileInfoLabel.height

        TapHandler {
            onTapped: Settings.openVideoExternal ? room.openMedia(eventId) : mediaControls.showControls()
        }

        Image {
            anchors.fill: parent
            source: thumbnailUrl ? thumbnailUrl.replace("mxc://", "image://MxcImage/") + "?scale" : "image://colorimage/:/icons/icons/ui/video-file.svg?" + palette.windowText
            asynchronous: true
            fillMode: Image.PreserveAspectFit

            VideoOutput {
                id: videoOutput

                visible: type == MtxEvent.VideoMessage
                clip: true
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
                orientation: mxcmedia.orientation
            }

        }

        MediaControls {
            id: mediaControls

            anchors.left: videoContainer.left
            anchors.right: videoContainer.right
            anchors.bottom: videoContainer.bottom
            playingVideo: type == MtxEvent.VideoMessage
            positionValue: mxcmedia.position
            duration: mediaLoaded ? mxcmedia.duration : content.duration
            mediaLoaded: mxcmedia.loaded
            mediaState: mxcmedia.playbackState
            onPositionChanged: mxcmedia.position = position
            onPlayPauseActivated: mxcmedia.playbackState == MediaPlayer.PlayingState ? mxcmedia.pause() : mxcmedia.play()
            onLoadActivated: mxcmedia.eventId = eventId
        }
    }

    // information about file name and file size
    Label {
        id: fileInfoLabel

        anchors.top: videoContainer.bottom
        text: body + " [" + filesize + "]"
        textFormat: Text.RichText
        elide: Text.ElideRight
        color: palette.text

        background: Rectangle {
            color: palette.base
        }

    }

}
