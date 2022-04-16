// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.3
import im.nheko

AbstractButton {
    required property string blurhash
    required property string body
    property double divisor: isReply ? 5 : 3
    required property string eventId
    required property string filename
    property bool fitsMetadata: (parent.width - width) > metadataWidth + 4
    required property bool isReply
    property int metadataWidth
    required property int originalWidth
    required property double proportionalHeight
    property int tempWidth: originalWidth < 1 ? 400 : originalWidth
    required property int type
    required property string url

    height: width * proportionalHeight
    hoverEnabled: true
    implicitWidth: Math.round(tempWidth * Math.min((timelineView.height / divisor) / (tempWidth * proportionalHeight), 1))
    width: Math.min(parent?.width ?? implicitWidth, implicitWidth)

    onClicked: Settings.openImageExternal ? room.openMedia(eventId) : TimelineManager.openImageOverlay(room, url, eventId)

    Image {
        id: blurhash_
        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: blurhash ? ("image://blurhash/" + blurhash) : ("image://colorimage/:/icons/icons/ui/image-failed.svg?" + timelineRoot.palette.placeholderText)
        sourceSize.height: parent.height * Screen.devicePixelRatio
        sourceSize.width: parent.width * Screen.devicePixelRatio
        visible: img.status != Image.Ready
    }
    Image {
        id: img
        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        mipmap: true
        smooth: true
        source: url.replace("mxc://", "image://MxcImage/") + "?scale"
        sourceSize.height: Math.min(Screen.desktopAvailableHeight, (originalWidth < 1 ? Screen.desktopAvailableHeight : originalWidth * proportionalHeight)) * Screen.devicePixelRatio
        sourceSize.width: Math.min(Screen.desktopAvailableWidth, originalWidth < 1 ? Screen.desktopAvailableWidth : originalWidth) * Screen.devicePixelRatio
        visible: !mxcimage.loaded
    }
    MxcAnimatedImage {
        id: mxcimage
        anchors.fill: parent
        eventId: parent.eventId
        play: !Settings.animateImagesOnHover || parent.hovered
        roomm: room
        visible: loaded
    }
    Item {
        id: overlay
        anchors.fill: parent
        visible: parent.hovered

        Rectangle {
            id: container
            anchors.bottom: overlay.bottom
            color: timelineRoot.palette.window
            implicitHeight: imgcaption.implicitHeight
            opacity: 0.75
            width: parent.width
        }
        Text {
            id: imgcaption
            anchors.fill: container
            color: timelineRoot.palette.text
            elide: Text.ElideMiddle
            horizontalAlignment: Text.AlignHCenter
            // See this MSC: https://github.com/matrix-org/matrix-doc/pull/2530
            text: filename ? filename : body
            verticalAlignment: Text.AlignVCenter
        }
    }
}
