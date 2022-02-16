// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
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
    property double divisor: isReply ? 5 : 3

    implicitWidth: Math.round(originalWidth*Math.min((timelineView.height/divisor)/(originalWidth*proportionalHeight), 1))
    width: Math.min(parent.width,implicitWidth)
    height: width*proportionalHeight

    property int metadataWidth
    property bool fitsMetadata: (parent.width - width) > metadataWidth+4

    Image {
        id: blurhash_

        anchors.fill: parent
        visible: img.status != Image.Ready
        source: blurhash ? ("image://blurhash/" + blurhash) : ("image://colorimage/:/icons/icons/ui/image-failed.svg?" + Nheko.colors.buttonText)
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        sourceSize.width: parent.width * Screen.devicePixelRatio
        sourceSize.height: parent.height * Screen.devicePixelRatio
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

        sourceSize.width: Math.min(Screen.desktopAvailableWidth, originalWidth < 1 ? Screen.desktopAvailableWidth : originalWidth) * Screen.devicePixelRatio
        sourceSize.height: Math.min(Screen.desktopAvailableHeight, (originalWidth < 1 ? Screen.desktopAvailableHeight : originalWidth*proportionalHeight)) * Screen.devicePixelRatio
    }

    MxcAnimatedImage {
        id: mxcimage

        visible: loaded
        anchors.fill: parent
        roomm: room
        play: !Settings.animateImagesOnHover || mouseArea.hovered
        eventId: parent.eventId
    }

    TapHandler {
        //enabled: type == MtxEvent.ImageMessage && (img.status == Image.Ready || mxcimage.loaded)
        onSingleTapped: {
            TimelineManager.openImageOverlay(room, url, eventId);
            eventPoint.accepted = true;
        }
        gesturePolicy: TapHandler.ReleaseWithinBounds
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
