// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
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

    margins: 2
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: stickersPerRow * stickerDimPad + 20

    Rectangle {
        color: Nheko.colors.window
        height: columnView.implicitHeight + Nheko.paddingSmall*2
        width: stickersPerRow * stickerDimPad + 20

        ColumnLayout {
            id: columnView

            spacing: Nheko.paddingSmall
            anchors.leftMargin: Nheko.paddingSmall
            anchors.rightMargin: Nheko.paddingSmall
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            // Search field
            TextField {
                id: emojiSearch

                Layout.preferredWidth: stickersPerRow * stickerDimPad + 20 - Nheko.paddingSmall
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

                ImageButton {
                    id: clearSearch

                    visible: emojiSearch.text !== ''

                    image: ":/icons/icons/ui/round-remove-button.svg"
                    focusPolicy: Qt.NoFocus
                    onClicked: emojiSearch.clear()
                    hoverEnabled: true
                    anchors {
                        top: parent.top
                        bottom: parent.bottom
                        right: parent.right
                        rightMargin: Nheko.paddingSmall
                    }
                }
            }

            // emoji grid
            GridView {
                id: gridView

                model: roomid ? TimelineManager.completerFor("stickers", roomid) : null
                Layout.preferredHeight: cellHeight * 3.5
                Layout.preferredWidth: stickersPerRow * stickerDimPad + 20 - Nheko.paddingSmall
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
