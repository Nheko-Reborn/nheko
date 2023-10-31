// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import im.nheko

AbstractButton {
    required property string blurhash
    required property string body
    required property int containerHeight
    property double divisor: EventDelegateChooser.isReply ? 10 : 4
    required property string eventId
    required property string filename
    property bool fitsMetadata: parent != null ? (parent.width - width) > metadataWidth + 4 : false
    property int metadataWidth
    required property int originalHeight
    required property int originalWidth
    required property double proportionalHeight
    required property int type
    required property string url

    EventDelegateChooser.aspectRatio: proportionalHeight
    EventDelegateChooser.keepAspectRatio: true
    EventDelegateChooser.maxHeight: containerHeight / divisor
    EventDelegateChooser.maxWidth: originalWidth
    enabled: !EventDelegateChooser.isReply
    hoverEnabled: true
    state: (img.status != Image.Ready || timeline.privacyScreen.active) ? "BlurhashVisible" : "ImageVisible"

    states: [
        State {
            name: "BlurhashVisible"

            PropertyChanges {
                blurhash_ {
                    opacity: (img.status != Image.Ready) || (timeline.privacyScreen.active && blurhash) ? 1 : 0
                    visible: (img.status != Image.Ready) || (timeline.privacyScreen.active && blurhash)
                }
            }
            PropertyChanges {
                img.opacity: 0
            }
            PropertyChanges {
                mxcimage.opacity: 0
            }
        },
        State {
            name: "ImageVisible"

            PropertyChanges {
                blurhash_ {
                    opacity: 0
                    visible: false
                }
            }
            PropertyChanges {
                img.opacity: 1
            }
            PropertyChanges {
                mxcimage.opacity: 1
            }
        }
    ]
    transitions: [
        Transition {
            from: "ImageVisible"
            reversible: true
            to: "BlurhashVisible"

            SequentialAnimation {
                PropertyAction {
                    property: "visible"
                    target: blurhash_
                }
                ParallelAnimation {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.Linear
                        property: "opacity"
                        target: blurhash_
                    }
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.Linear
                        property: "opacity"
                        target: img
                    }
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.Linear
                        property: "opacity"
                        target: mxcimage
                    }
                }
            }
        }
    ]

    onClicked: Settings.openImageExternal ? room.openMedia(eventId) : TimelineManager.openImageOverlay(room, url, eventId, originalWidth, proportionalHeight)

    Image {
        id: img

        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        horizontalAlignment: Image.AlignLeft
        mipmap: true
        smooth: true
        source: url != "" ? (url.replace("mxc://", "image://MxcImage/") + "?scale") : ""
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
    Image {
        id: blurhash_

        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: blurhash ? ("image://blurhash/" + blurhash) : ("image://colorimage/:/icons/icons/ui/image-failed.svg?" + palette.buttonText)
        sourceSize.height: parent.height * Screen.devicePixelRatio
        sourceSize.width: parent.width * Screen.devicePixelRatio
    }
    Item {
        id: overlay

        anchors.fill: parent
        visible: parent.hovered

        Rectangle {
            id: container

            anchors.bottom: overlay.bottom
            color: palette.window
            implicitHeight: imgcaption.implicitHeight
            opacity: 0.75
            width: parent.width
        }
        Text {
            id: imgcaption

            anchors.fill: container
            color: palette.text
            elide: Text.ElideMiddle
            horizontalAlignment: Text.AlignHCenter
            // See this MSC: https://github.com/matrix-org/matrix-doc/pull/2530
            text: filename ? filename : body
            verticalAlignment: Text.AlignVCenter
        }
    }
}
