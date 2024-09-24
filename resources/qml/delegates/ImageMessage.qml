// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import im.nheko

AbstractButton {
    required property int type
    required property int originalWidth
    required property int originalHeight
    required property double proportionalHeight
    required property string url
    required property string blurhash
    required property string body
    required property string filename
    required property string eventId
    required property int containerHeight
    property double divisor: EventDelegateChooser.isReply ? 10 : 4

    property bool showImage: room.showImage()

    EventDelegateChooser.keepAspectRatio: true
    EventDelegateChooser.maxWidth: originalWidth
    EventDelegateChooser.maxHeight: containerHeight / divisor
    EventDelegateChooser.aspectRatio: proportionalHeight

    hoverEnabled: true
    enabled: !EventDelegateChooser.isReply

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
            to: "BlurhashVisible"
            reversible: true
            enabled: !Settings.reducedMotion

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
        source: (url != "" && showImage) ? (url.replace("mxc://", "image://MxcImage/") + "?scale") : ""
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        horizontalAlignment: Image.AlignLeft
        smooth: true
        mipmap: true

        sourceSize.width: Math.min(Screen.desktopAvailableWidth, originalWidth < 1 ? Screen.desktopAvailableWidth : originalWidth) * Screen.devicePixelRatio
        sourceSize.height: Math.min(Screen.desktopAvailableHeight, (originalWidth < 1 ? Screen.desktopAvailableHeight : originalWidth*proportionalHeight)) * Screen.devicePixelRatio
    }

    MxcAnimatedImage {
        id: mxcimage

        visible: loaded
        roomm: room
        play: !Settings.animateImagesOnHover || parent.hovered
        eventId: showImage ? parent.eventId : ""

        anchors.fill: parent
    }

    Image {
        id: blurhash_

        source: blurhash ? ("image://blurhash/" + blurhash) : ("image://colorimage/:/icons/icons/ui/image-failed.svg?" + palette.buttonText)
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        sourceSize.width: blurhash ? parent.width * Screen.devicePixelRatio : Math.min(parent.width, parent.height)
        sourceSize.height: blurhash ? parent.height * Screen.devicePixelRatio : Math.min(parent.width, parent.height)

        anchors.fill: parent
    }

    onClicked: {
        Settings.openImageExternal ? room.openMedia(eventId) : TimelineManager.openImageOverlay(room, url, eventId, originalWidth, proportionalHeight);
    }

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

    Button {
        anchors.centerIn: parent
        visible: !showImage && !parent.EventDelegateChooser.isReply
        enabled: visible
        text: qsTr("Show")
        onClicked: {
            showImage = true;
        }
    }
}
