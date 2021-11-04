// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import im.nheko 1.0

Item {
    required property int type
    required property int originalWidth
    required property double proportionalHeight
    required property string url
    required property string blurhash
    required property string body
    required property string filename
    required property bool isReply
    required property string eventId
    property double tempWidth: Math.min(parent ? parent.width : undefined, originalWidth < 1 ? 200 : originalWidth)
    property double tempHeight: tempWidth * proportionalHeight
    property double divisor: isReply ? 5 : 3
    property bool tooHigh: tempHeight > timelineView.height / divisor

    height: Math.round(tooHigh ? timelineView.height / divisor : tempHeight)
    width: Math.round(tooHigh ? (timelineView.height / divisor) / proportionalHeight : tempWidth)

    Image {
        id: blurhash_

        anchors.fill: parent
        visible: img.status != Image.Ready
        source: blurhash ? ("image://blurhash/" + blurhash) : ("image://colorimage/:/icons/icons/ui/do-not-disturb-rounded-sign@2x.png?" + Nheko.colors.buttonText)
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        sourceSize.width: parent.width
        sourceSize.height: parent.height
    }

    Image {
        id: img

        visible: !mxcimage.loaded
        anchors.fill: parent
        source: url.replace("mxc://", "image://MxcImage/")
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true

        TapHandler {
            enabled: type == MtxEvent.ImageMessage && img.status == Image.Ready
            onSingleTapped: {
                TimelineManager.openImageOverlay(url, room.data.eventId);
                eventPoint.accepted = true;
            }
            gesturePolicy: TapHandler.ReleaseWithinBounds
        }

    }

    MxcAnimatedImage {
        id: mxcimage

        visible: loaded
        anchors.fill: parent
        roomm: room
        play: !Settings.animateImagesOnHover || mouseArea.hovered
        eventId: parent.eventId
    }

    HoverHandler {
        id: mouseArea
    }

    Item {
        id: overlay

        anchors.fill: parent
        visible: mouseArea.hovered

        Rectangle {
            id: container

            width: parent.width
            implicitHeight: imgcaption.implicitHeight
            anchors.bottom: overlay.bottom
            color: Nheko.colors.window
            opacity: 0.75
        }

        Text {
            id: imgcaption

            anchors.fill: container
            elide: Text.ElideMiddle
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            // See this MSC: https://github.com/matrix-org/matrix-doc/pull/2530
            text: filename ? filename : body
            color: Nheko.colors.text
        }

    }

}
