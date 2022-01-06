// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import Qt.labs.animation 1.0

import ".."

import im.nheko 1.0

Window {
    id: imageOverlay

    required property string url
    required property string eventId
    required property Room room

    flags: Qt.FramelessWindowHint

    visibility: Window.FullScreen
    color: Qt.rgba(0.2,0.2,0.2,0.66)

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: imageOverlay.close()
    }


    Item {
        height: Math.min(parent.height, img.implicitHeight)
        width: Math.min(parent.width, img.implicitWidth)
        x: (parent.width - img.width)/2
        y: (parent.height - img.height)/2

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

        BoundaryRule on scale {
            enabled: img.loaded || mxcimage.loaded
            id: sbr
            minimum: 0.1
            maximum: 10
            minimumOvershoot: 0.02; maximumOvershoot: 10.02
        }

        BoundaryRule on x {
           enabled: img.loaded || mxcimage.loaded
           id: xbr
           minimum: -100
           maximum: imageOverlay.width - img.width + 100
           minimumOvershoot: 100; maximumOvershoot: 100
           overshootFilter: BoundaryRule.Peak
        }

        BoundaryRule on y {
           enabled: img.loaded || mxcimage.loaded
           id: ybr
           minimum: -100
           maximum: imageOverlay.height - img.height + 100
           minimumOvershoot: 100; maximumOvershoot: 100
           overshootFilter: BoundaryRule.Peak
        }

        PinchHandler {
            onActiveChanged: if (!active) sbr.returnToBounds();
        }

        WheelHandler {
            property: "scale"
            onActiveChanged: if (!active) sbr.returnToBounds();
        }

        DragHandler {
            onActiveChanged: if (!active) {
               xbr.returnToBounds();
               ybr.returnToBounds();
            }
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
