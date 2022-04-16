// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import im.nheko

Menu {
    id: stickerPopup

    property var callback
    property var colors
    property real highlightHue: timelineRoot.palette.highlight.hslHue
    property real highlightLight: timelineRoot.palette.highlight.hslLightness
    property real highlightSat: timelineRoot.palette.highlight.hslSaturation
    property alias model: gridView.model
    property string roomid
    readonly property int stickerDim: 128
    readonly property int stickerDimPad: 128 + Nheko.paddingSmall
    readonly property int stickersPerRow: 3
    property var textArea

    function show(showAt, roomid_, callback) {
        console.debug("Showing sticker picker");
        roomid = roomid_;
        stickerPopup.callback = callback;
        popup(showAt ? showAt : null);
    }

    bottomPadding: 1
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true
    leftPadding: 1
    margins: 0
    modal: true
    rightPadding: 1
    width: stickersPerRow * stickerDimPad + 20

    Rectangle {
        color: timelineRoot.palette.window
        height: columnView.implicitHeight + 4
        width: stickersPerRow * stickerDimPad + 20

        ColumnLayout {
            id: columnView
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 3
            anchors.right: parent.right
            anchors.rightMargin: 3
            anchors.topMargin: 2
            spacing: 0

            // Search field
            TextField {
                id: emojiSearch
                Layout.preferredWidth: stickersPerRow * stickerDimPad + 20 - 6
                Layout.topMargin: 3
                background: null
                color: timelineRoot.palette.text
                palette: timelineRoot.palette
                placeholderText: qsTr("Search")
                placeholderTextColor: timelineRoot.palette.placeholderText
                rightPadding: clearSearch.width
                selectByMouse: true

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
                    background: null
                    focusPolicy: Qt.NoFocus
                    hoverEnabled: true
                    icon.source: "image://colorimage/:/icons/icons/ui/round-remove-button.svg?" + (clearSearch.hovered ? timelineRoot.palette.highlight : timelineRoot.palette.placeholderText)
                    visible: emojiSearch.text !== ''

                    onClicked: emojiSearch.clear()

                    anchors {
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                    }
                    // clear the default hover effects.
                    Image {
                        height: parent.height - 2 * Nheko.paddingSmall
                        source: "image://colorimage/:/icons/icons/ui/round-remove-button.svg?" + (clearSearch.hovered ? timelineRoot.palette.highlight : timelineRoot.palette.placeholderText)
                        width: height

                        anchors {
                            margins: Nheko.paddingSmall
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }

            // emoji grid
            GridView {
                id: gridView
                Layout.leftMargin: 4
                Layout.preferredHeight: cellHeight * 3.5
                Layout.preferredWidth: stickersPerRow * stickerDimPad + 20
                boundsBehavior: Flickable.StopAtBounds
                cacheBuffer: 500
                cellHeight: stickerDimPad
                cellWidth: stickerDimPad
                clip: true
                currentIndex: -1 // prevent sorting from stealing focus
                model: roomid ? TimelineManager.completerFor("stickers", roomid) : null

                ScrollBar.vertical: ScrollBar {
                    id: emojiScroll
                }

                // Individual emoji
                delegate: AbstractButton {
                    ToolTip.text: ":" + model.shortcode + ": - " + model.body
                    ToolTip.visible: hovered
                    height: stickerDim
                    hoverEnabled: true
                    width: stickerDim

                    background: Rectangle {
                        anchors.fill: parent
                        color: hovered ? timelineRoot.palette.highlight : 'transparent'
                        radius: 5
                    }
                    contentItem: Image {
                        fillMode: Image.PreserveAspectFit
                        height: stickerDim
                        source: model.url.replace("mxc://", "image://MxcImage/") + "?scale"
                        width: stickerDim
                    }

                    // TODO: maybe add favorites at some point?
                    onClicked: {
                        console.debug("Picked " + model.shortcode);
                        stickerPopup.close();
                        callback(model.originalRow);
                    }
                }
            }
        }
    }
}
