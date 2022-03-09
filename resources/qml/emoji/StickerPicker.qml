// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtGraphicalEffects 1.0
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import im.nheko 1.0
import im.nheko.EmojiModel 1.0

Menu {
    id: stickerPopup

    property var callback
    property var colors
    property string roomid
    property alias model: gridView.model
    property var textArea
    property real highlightHue: Nheko.colors.highlight.hslHue
    property real highlightSat: Nheko.colors.highlight.hslSaturation
    property real highlightLight: Nheko.colors.highlight.hslLightness
    readonly property int stickerDim: 128
    readonly property int stickerDimPad: 128 + Nheko.paddingSmall
    readonly property int stickersPerRow: 3

    function show(showAt, roomid_, callback) {
        console.debug("Showing sticker picker");
        roomid = roomid_;
        stickerPopup.callback = callback;
        popup(showAt ? showAt : null);
    }

    margins: 0
    bottomPadding: 1
    leftPadding: 1
    rightPadding: 1
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: stickersPerRow * stickerDimPad + 20

    Rectangle {
        color: Nheko.colors.window
        height: columnView.implicitHeight + 4
        width: stickersPerRow * stickerDimPad + 20

        ColumnLayout {
            id: columnView

            spacing: 0
            anchors.leftMargin: 3
            anchors.rightMargin: 3
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 2

            // Search field
            TextField {
                id: emojiSearch

                Layout.topMargin: 3
                Layout.preferredWidth: stickersPerRow * stickerDimPad + 20 - 6
                palette: Nheko.colors
                background: null
                placeholderTextColor: Nheko.colors.buttonText
                color: Nheko.colors.text
                placeholderText: qsTr("Search")
                selectByMouse: true
                rightPadding: clearSearch.width
                onTextChanged: searchTimer.restart()
                onVisibleChanged: {
                    if (visible)
                        forceActiveFocus();
                    else
                        clear();
                }

                Timer {
                    id: searchTimer

                    interval: 350 // tweak as needed?
                    onTriggered: stickerPopup.model.searchString = emojiSearch.text
                }

                ToolButton {
                    id: clearSearch

                    visible: emojiSearch.text !== ''
                    icon.source: "image://colorimage/:/icons/icons/ui/round-remove-button.svg?" + (clearSearch.hovered ? Nheko.colors.highlight : Nheko.colors.buttonText)
                    focusPolicy: Qt.NoFocus
                    onClicked: emojiSearch.clear()
                    hoverEnabled: true
                    background: null

                    anchors {
                        verticalCenter: parent.verticalCenter
                        right: parent.right
                    }
                    // clear the default hover effects.

                    Image {
                        height: parent.height - 2 * Nheko.paddingSmall
                        width: height
                        source: "image://colorimage/:/icons/icons/ui/round-remove-button.svg?" + (clearSearch.hovered ? Nheko.colors.highlight : Nheko.colors.buttonText)

                        anchors {
                            verticalCenter: parent.verticalCenter
                            right: parent.right
                            margins: Nheko.paddingSmall
                        }

                    }

                }

            }

            // emoji grid
            GridView {
                id: gridView

                model: roomid ? TimelineManager.completerFor("stickers", roomid) : null
                Layout.preferredHeight: cellHeight * 3.5
                Layout.preferredWidth: stickersPerRow * stickerDimPad + 20
                Layout.leftMargin: 4
                cellWidth: stickerDimPad
                cellHeight: stickerDimPad
                boundsBehavior: Flickable.StopAtBounds
                clip: true
                currentIndex: -1 // prevent sorting from stealing focus
                cacheBuffer: 500

                ScrollHelper {
                    flickable: parent
                    anchors.fill: parent
                    enabled: !Settings.mobileMode
                }

                // Individual emoji
                delegate: AbstractButton {
                    width: stickerDim
                    height: stickerDim
                    hoverEnabled: true
                    ToolTip.text: ":" + model.shortcode + ": - " + model.body
                    ToolTip.visible: hovered
                    // TODO: maybe add favorites at some point?
                    onClicked: {
                        console.debug("Picked " + model.shortcode);
                        stickerPopup.close();
                        callback(model.originalRow);
                    }

                    contentItem: Image {
                        height: stickerDim
                        width: stickerDim
                        source: model.url.replace("mxc://", "image://MxcImage/") + "?scale"
                        fillMode: Image.PreserveAspectFit
                    }

                    background: Rectangle {
                        anchors.fill: parent
                        color: hovered ? Nheko.colors.highlight : 'transparent'
                        radius: 5
                    }

                }

                ScrollBar.vertical: ScrollBar {
                    id: emojiScroll
                }

            }

        }

    }

}
