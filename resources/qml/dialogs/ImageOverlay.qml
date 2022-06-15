// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15

import ".."

import im.nheko 1.0

Window {
    id: imageOverlay

    required property string url
    required property string eventId
    required property Room room
    required property int originalWidth
    required property double proportionalHeight

    flags: Qt.FramelessWindowHint

    //visibility: Window.FullScreen
    color: Qt.rgba(0.2,0.2,0.2,0.66)

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: imageOverlay.close()
    }

    TapHandler {
        onSingleTapped: imageOverlay.close();
    }


    Item {
        id: imgContainer

        property int imgSrcWidth: (imageOverlay.originalWidth && imageOverlay.originalWidth > 100) ? imageOverlay.originalWidth : Screen.width
        property int imgSrcHeight: imageOverlay.proportionalHeight ? imgSrcWidth * imageOverlay.proportionalHeight : Screen.height

        height: Math.min(parent.height || Screen.height, imgSrcHeight)
        width: Math.min(parent.width || Screen.width, imgSrcWidth)

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        Image {
            id: img

            visible: !mxcimage.loaded
            anchors.fill: parent
            source: url.replace("mxc://", "image://MxcImage/")
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            property bool loaded: status == Image.Ready
        }

        MxcAnimatedImage {
            id: mxcimage

            visible: loaded
            anchors.fill: parent
            roomm: imageOverlay.room
            play: !Settings.animateImagesOnHover || mouseArea.hovered
            eventId: imageOverlay.eventId
        }

        onScaleChanged: {
            if (scale > 10) scale = 10;
            if (scale < 0.1) scale = 0.1
        }
    }

    Item {
        anchors.fill: parent


        PinchHandler {
            target: imgContainer
            maximumScale: 10
            minimumScale: 0.1
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
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Nheko.paddingLarge
        spacing: Nheko.paddingMedium

        ImageButton {
            height: 48
            width: 48
            hoverEnabled: true
            image: ":/icons/icons/ui/download.svg"
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
            width: 48
            hoverEnabled: true
            image: ":/icons/icons/ui/dismiss.svg"
            //ToolTip.visible: hovered
            //ToolTip.delay: Nheko.tooltipDelay
            //ToolTip.text: qsTr("Close")
            onClicked: imageOverlay.close()
        }
    }

}
