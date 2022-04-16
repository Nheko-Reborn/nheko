// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Window 2.15
import "../"
import im.nheko

Window {
    id: imageOverlay

    required property string eventId
    required property int originalWidth
    required property double proportionalHeight
    required property Room room
    required property string url

    //visibility: Window.FullScreen
    color: Qt.rgba(0.2, 0.2, 0.2, 0.66)
    flags: Qt.FramelessWindowHint

    Shortcut {
        sequence: StandardKey.Cancel

        onActivated: imageOverlay.close()
    }
    Item {
        id: imgContainer

        property int imgSrcHeight: proportionalHeight ? imgSrcWidth * proportionalHeight : Screen.height
        property int imgSrcWidth: (originalWidth && originalWidth > 200) ? originalWidth : Screen.width

        height: Math.min(parent.height, imgSrcHeight)
        width: Math.min(parent.width, imgSrcWidth)
        x: (parent.width - width)
        y: (parent.height - height)

        onScaleChanged: {
            if (scale > 10)
                scale = 10;
            if (scale < 0.1)
                scale = 0.1;
        }

        Image {
            id: img

            property bool loaded: status == Image.Ready

            anchors.fill: parent
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            mipmap: true
            smooth: true
            source: url.replace("mxc://", "image://MxcImage/")
            visible: !mxcimage.loaded
        }
        MxcAnimatedImage {
            id: mxcimage
            anchors.fill: parent
            eventId: imageOverlay.eventId
            play: !Settings.animateImagesOnHover || mouseArea.hovered
            roomm: imageOverlay.room
            visible: loaded
        }
    }
    Item {
        anchors.fill: parent

        PinchHandler {
            maximumScale: 10
            minimumScale: 0.1
            target: imgContainer
        }
        WheelHandler {
            property: "scale"
            target: imgContainer
        }
        DragHandler {
            target: imgContainer
        }
        HoverHandler {
            id: mouseArea
        }
    }
    Row {
        anchors.margins: Nheko.paddingLarge
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Nheko.paddingMedium

        ImageButton {
            height: 48
            hoverEnabled: true
            image: ":/icons/icons/ui/download.svg"
            width: 48

            //ToolTip.visible: hovered
            //ToolTip.delay: Nheko.tooltipDelay
            //ToolTip.text: qsTr("Download")
            onClicked: {
                if (room) {
                    room.saveMedia(eventId);
                } else {
                    TimelineManager.saveMedia(url);
                }
                imageOverlay.close();
            }
        }
        ImageButton {
            height: 48
            hoverEnabled: true
            image: ":/icons/icons/ui/dismiss.svg"
            width: 48

            //ToolTip.visible: hovered
            //ToolTip.delay: Nheko.tooltipDelay
            //ToolTip.text: qsTr("Close")
            onClicked: imageOverlay.close()
        }
    }
}
