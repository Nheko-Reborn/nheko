// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko
import im.nheko.EmojiModel

Menu {
    id: stickerPopup

    property var callback
    property string roomid
    property alias model: gridView.model
    required property bool emoji
    property var textArea
    property real highlightHue: palette.highlight.hslHue
    property real highlightSat: palette.highlight.hslSaturation
    property real highlightLight: palette.highlight.hslLightness
    readonly property int stickerDim: emoji ? 48 : 128
    readonly property int stickerDimPad: stickerDim + Nheko.paddingSmall
    readonly property int stickersPerRow: emoji ? 7 : 3
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
        color: palette.window
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
                background: null
                placeholderTextColor: palette.buttonText
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

                model: roomid ? TimelineManager.completerFor(stickerPopup.emoji ? "emojigrid" : "stickergrid", roomid) : null
                Layout.row: 1
                Layout.column: 1
                Layout.preferredHeight: cellHeight * (stickersPerRow + 0.5)
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
                    color: palette.alternateBase

                    required property string section

                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        text: parent.section
                        font.bold: true
                    }
                }
                section.labelPositioning: ViewSection.InlineLabels | ViewSection.CurrentLabelAtStart

                spacing: Nheko.paddingSmall

                // Individual emoji
                delegate: Row {
                    required property var row;

                    spacing: Nheko.paddingSmall

                    Repeater {
                        model: row

                        delegate: AbstractButton {
                            id: del

                            required property var modelData

                            width: stickerDim
                            height: stickerDim
                            hoverEnabled: true
                            ToolTip.text: ":" + modelData.shortcode + ": - " + (modelData.unicode ? modelData.unicodeName : modelData.body)
                            ToolTip.visible: hovered
                            // TODO: maybe add favorites at some point?
                            onClicked: {
                                console.debug("Picked " + modelData);
                                stickerPopup.close();
                                if (!stickerPopup.emoji) {
                                    // return descriptor to calculate sticker to send
                                    callback(modelData.descriptor);
                                } else if (modelData.unicode) {
                                    // return the emoji unicode as both plain text and markdown
                                    callback(modelData.unicode, modelData.unicode);
                                } else {
                                    // return the emoji url as plain text and a markdown link as markdown
                                    callback(modelData.url, modelData.markdown);
                                }
                            }

                            contentItem: DelegateChooser {
                                roleValue: del.modelData.unicode != undefined

                                DelegateChoice {
                                    roleValue: true

                                    Text {
                                        width: stickerDim
                                        height: stickerDim
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.family: Settings.emojiFont
                                        font.pixelSize: 36
                                        text: del.modelData.unicode.replace('\ufe0f', '')
                                    }
                                }

                                DelegateChoice {
                                    roleValue: false
                                    Image {
                                        height: stickerDim
                                        width: stickerDim
                                        source: del.modelData.url.replace("mxc://", "image://MxcImage/") + "?scale"
                                        fillMode: Image.PreserveAspectFit
                                    }
                                }
                            }

                            background: Rectangle {
                                anchors.fill: parent
                                color: hovered ? palette.highlight : 'transparent'
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
                clip: true

                delegate: Avatar {
                    height: sidebarAvatarSize
                    width: sidebarAvatarSize
                    url: modelData.url.replace("mxc://", "image://MxcImage/")
                    textColor: modelData.url.startsWith("mxc://") ? palette.text : palette.buttonText
                    displayName: modelData.name
                    roomid: modelData.name

                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: modelData.name
                    onClicked: gridView.positionViewAtIndex(modelData.firstRowWith, ListView.Beginning)
                }
            }

            ImageButton {
                Layout.row: 0
                Layout.column: 0
                Layout.preferredWidth: sidebarAvatarSize
                Layout.preferredHeight: sidebarAvatarSize
                Layout.rightMargin: Nheko.paddingSmall

                image: ":/icons/icons/ui/settings.svg"

                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.delay: Nheko.tooltipDelay
                ToolTip.text: qsTr("Change what packs are enabled, remove packs, or create new ones")
                onClicked: TimelineManager.openImagePackSettings(stickerPopup.roomid)
            }
        }

    }

}
