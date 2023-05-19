// SPDX-FileCopyrightText: Nheko Contributors
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
    readonly property int sidebarAvatarSize: 24

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
    width: sidebarAvatarSize + Nheko.paddingSmall + stickersPerRow * stickerDimPad + 20

    Rectangle {
        color: Nheko.colors.window
        height: columnView.implicitHeight + Nheko.paddingSmall*2
        width: sidebarAvatarSize + Nheko.paddingSmall + stickersPerRow * stickerDimPad + 20

        GridLayout {
            id: columnView

            anchors.leftMargin: Nheko.paddingSmall
            anchors.rightMargin: Nheko.paddingSmall
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            columns: 2
            rows: 2

            // Search field
            TextField {
                id: emojiSearch

                Layout.preferredWidth: stickersPerRow * stickerDimPad + 20 - Nheko.paddingSmall
                Layout.row: 0
                Layout.column: 1
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

            // sticker grid
            ListView {
                id: gridView

                model: roomid ? TimelineManager.completerFor("stickergrid", roomid) : null
                Layout.row: 1
                Layout.column: 1
                Layout.preferredHeight: cellHeight * 3.5
                Layout.preferredWidth: stickersPerRow * stickerDimPad + 20 - Nheko.paddingSmall
                property int cellHeight: stickerDimPad
                boundsBehavior: Flickable.StopAtBounds
                clip: true
                currentIndex: -1 // prevent sorting from stealing focus

                section.property: "packname"
                section.criteria: ViewSection.FullString
                section.delegate: Rectangle {
                    width: gridView.width
                    height: childrenRect.height
                    color: Nheko.colors.alternateBase

                    required property string section

                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        text: parent.section
                        color: Nheko.colors.text
                        font.bold: true
                    }
                }
                section.labelPositioning: ViewSection.InlineLabels | ViewSection.CurrentLabelAtStart

                spacing: Nheko.paddingSmall

                ScrollHelper {
                    flickable: parent
                    anchors.fill: parent
                    enabled: !Settings.mobileMode
                }

                // Individual emoji
                delegate: Row {
                    required property var row;

                    spacing: Nheko.paddingSmall

                    Repeater {
                        model: row

                        delegate: AbstractButton {
                            width: stickerDim
                            height: stickerDim
                            hoverEnabled: true
                            ToolTip.text: ":" + modelData.shortcode + ": - " + modelData.body
                            ToolTip.visible: hovered
                            // TODO: maybe add favorites at some point?
                            onClicked: {
                                console.debug("Picked " + modelData.descriptor);
                                stickerPopup.close();
                                callback(modelData.descriptor);
                            }

                            contentItem: Image {
                                height: stickerDim
                                width: stickerDim
                                source: modelData.url.replace("mxc://", "image://MxcImage/") + "?scale"
                                fillMode: Image.PreserveAspectFit
                            }

                            background: Rectangle {
                                anchors.fill: parent
                                color: hovered ? Nheko.colors.highlight : 'transparent'
                                radius: 5
                            }

                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    id: emojiScroll
                }

            }

            ListView {
                Layout.row: 1
                Layout.column: 0
                Layout.preferredWidth: sidebarAvatarSize
                Layout.fillHeight: true
                Layout.rightMargin: Nheko.paddingSmall

                model: gridView.model ? gridView.model.sections : null
                spacing: Nheko.paddingSmall

                delegate: Avatar {
                    height: sidebarAvatarSize
                    width: sidebarAvatarSize
                    url: modelData.url.replace("mxc://", "image://MxcImage/")
                    displayName: modelData.name
                    roomid: modelData.name

                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: modelData.name
                    onClicked: gridView.positionViewAtIndex(modelData.firstRowWith, ListView.Beginning)
                }
            }
        }

    }

}
