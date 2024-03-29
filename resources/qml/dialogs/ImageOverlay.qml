// SPDX-FileCopyrightText: Nheko Contributors
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
        onSingleTapped: imageOverlay.close();
    }


    Item {
        id: imgContainer

        property int imgSrcWidth: (imageOverlay.originalWidth && imageOverlay.originalWidth > 100) ? imageOverlay.originalWidth : Screen.width
        property int imgSrcHeight: imageOverlay.proportionalHeight ? imgSrcWidth * imageOverlay.proportionalHeight : Screen.height

        property double initialScale: Math.min(Window.height/imgSrcHeight, Window.width/imgSrcWidth, 1.0)

        height: imgSrcHeight * initialScale
        width: imgSrcWidth * initialScale

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
            // workaround for QTBUG-87646 / QTBUG-112394 / QTBUG-112432:
            // Magic Mouse pretends to be a trackpad but doesn't work with PinchHandler
            // and we don't yet distinguish mice and trackpads on Wayland either
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
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
            image: ":/icons/icons/ui/copy.svg"

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
            width: 48
            hoverEnabled: true
            image: ":/icons/icons/ui/download.svg"

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
