// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import ".."
import im.nheko 1.0

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

    Component.onCompleted: Nheko.setWindowRole(imageOverlay, "imageoverlay")

    Shortcut {
        sequences: [StandardKey.Cancel]

        onActivated: imageOverlay.close()
    }
    Shortcut {
        sequences: [StandardKey.Copy]

        onActivated: {
            if (room) {
                room.copyMedia(eventId);
            } else {
                TimelineManager.copyImage(url);
            }
        }
    }
    TapHandler {
        onSingleTapped: imageOverlay.close()
    }
    Item {
        id: imgContainer

        property int imgSrcHeight: imageOverlay.proportionalHeight ? imgSrcWidth * imageOverlay.proportionalHeight : Screen.height
        property int imgSrcWidth: (imageOverlay.originalWidth && imageOverlay.originalWidth > 100) ? imageOverlay.originalWidth : Screen.width

        height: Math.min(parent.height || Screen.height, imgSrcHeight)
        width: Math.min(parent.width || Screen.width, imgSrcWidth)
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

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
            // workaround for QTBUG-87646 / QTBUG-112394 / QTBUG-112432:
            // Magic Mouse pretends to be a trackpad but doesn't work with PinchHandler
            // and we don't yet distinguish mice and trackpads on Wayland either
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
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
            image: ":/icons/icons/ui/copy.svg"
            width: 48

            //ToolTip.visible: hovered
            //ToolTip.delay: Nheko.tooltipDelay
            //ToolTip.text: qsTr("Copy to clipboard")

            onClicked: {
                imageOverlay.hide();
                if (room) {
                    room.copyMedia(eventId);
                } else {
                    TimelineManager.copyImage(url);
                }
                imageOverlay.close();
            }
        }
        ImageButton {
            height: 48
            hoverEnabled: true
            image: ":/icons/icons/ui/download.svg"
            width: 48

            //ToolTip.visible: hovered
            //ToolTip.delay: Nheko.tooltipDelay
            //ToolTip.text: qsTr("Download")

            onClicked: {
                imageOverlay.hide();
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
