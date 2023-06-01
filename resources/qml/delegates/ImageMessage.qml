// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.3
import im.nheko 1.0

AbstractButton {
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

    property int tempWidth: originalWidth < 1? 400: originalWidth

    implicitWidth: Math.round(tempWidth*Math.min((timelineView.height/divisor)/(tempWidth*proportionalHeight), 1))
    width: Math.min(parent?.width ?? 2000,implicitWidth)
    height: width*proportionalHeight
    hoverEnabled: true

    state: (img.status != Image.Ready || timeline.privacyScreen.active) ? "BlurhashVisible" : "ImageVisible"
    states: [
        State {
            name: "BlurhashVisible"

            PropertyChanges {
                target: blurhash_
                opacity: (img.status != Image.Ready) || (timeline.privacyScreen.active && blurhash) ? 1 : 0
                visible: (img.status != Image.Ready) || (timeline.privacyScreen.active && blurhash)
            }

            PropertyChanges {
                target: img
                opacity: 0
            }

            PropertyChanges {
                target: mxcimage
                opacity: 0
            }
        },
        State {
            name: "ImageVisible"

            PropertyChanges {
                target: blurhash_
                opacity: 0
                visible: false
            }

            PropertyChanges {
                target: img
                opacity: 1
            }

            PropertyChanges {
                target: mxcimage
                opacity: 1
            }
        }
    ]
    transitions: [
        Transition {
            from: "ImageVisible"
            to: "BlurhashVisible"
            reversible: true

            SequentialAnimation {
                PropertyAction {
                    target: blurhash_
                    property: "visible"
                }

                ParallelAnimation {
                    NumberAnimation {
                        target: blurhash_
                        property: "opacity"
                        duration: 300
                        easing.type: Easing.Linear
                    }

                    NumberAnimation {
                        target: img
                        property: "opacity"
                        duration: 300
                        easing.type: Easing.Linear
                    }

                    NumberAnimation {
                        target: mxcimage
                        property: "opacity"
                        duration: 300
                        easing.type: Easing.Linear
                    }
                }
            }
        }
    ]

    property int metadataWidth
    property bool fitsMetadata: parent != null ? (parent.width - width) > metadataWidth+4 : false

    Image {
        id: img

        visible: !mxcimage.loaded
        anchors.fill: parent
        source: url != "" ? (url.replace("mxc://", "image://MxcImage/") + "?scale") : ""
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
        play: !Settings.animateImagesOnHover || parent.hovered
        eventId: parent.eventId
    }

    Image {
        id: blurhash_

        anchors.fill: parent
        source: blurhash ? ("image://blurhash/" + blurhash) : ("image://colorimage/:/icons/icons/ui/image-failed.svg?" + palette.buttonText)
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        sourceSize.width: parent.width * Screen.devicePixelRatio
        sourceSize.height: parent.height * Screen.devicePixelRatio
    }

    onClicked: Settings.openImageExternal ? room.openMedia(eventId) : TimelineManager.openImageOverlay(room, url, eventId, originalWidth, proportionalHeight);

    Item {
        id: overlay

        anchors.fill: parent
        visible: parent.hovered

        Rectangle {
            id: container

            width: parent.width
            implicitHeight: imgcaption.implicitHeight
            anchors.bottom: overlay.bottom
            color: palette.window
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
            color: palette.text
        }

    }

}
