// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

Menu {
    id: stickerPopup

    property var callback
    required property bool emoji
    property real highlightHue: palette.highlight.hslHue
    property real highlightLight: palette.highlight.hslLightness
    property real highlightSat: palette.highlight.hslSaturation
    property alias model: gridView.model
    property string roomid
    readonly property int sidebarAvatarSize: 24
    readonly property int stickerDim: emoji ? 48 : 128
    readonly property int stickerDimPad: stickerDim + Nheko.paddingSmall
    readonly property int stickersPerRow: emoji ? 7 : 3
    property var textArea

    function show(showAt, roomid_, callback) {
        console.debug("Showing sticker picker");
        roomid = roomid_;
        stickerPopup.callback = callback;
        popup(showAt ? showAt : null);
    }

    bottomPadding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true
    leftPadding: 0
    margins: 2
    modal: true
    rightPadding: 0
    topPadding: 0
    width: sidebarAvatarSize + Nheko.paddingSmall + stickersPerRow * stickerDimPad + 20

    Rectangle {
        color: palette.window
        height: columnView.implicitHeight + Nheko.paddingSmall * 2
        width: sidebarAvatarSize + Nheko.paddingSmall + stickersPerRow * stickerDimPad + 20

        GridLayout {
            id: columnView

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: Nheko.paddingSmall
            anchors.right: parent.right
            anchors.rightMargin: Nheko.paddingSmall
            columns: 2
            rows: 2

            // Search field
            TextField {
                id: emojiSearch

                Layout.column: 1
                Layout.preferredWidth: stickersPerRow * stickerDimPad + 20 - Nheko.paddingSmall
                Layout.row: 0
                background: null
                placeholderText: qsTr("Search")
                placeholderTextColor: palette.buttonText
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
                ImageButton {
                    id: clearSearch

                    focusPolicy: Qt.NoFocus
                    hoverEnabled: true
                    image: ":/icons/icons/ui/round-remove-button.svg"
                    visible: emojiSearch.text !== ''

                    onClicked: emojiSearch.clear()

                    anchors {
                        bottom: parent.bottom
                        right: parent.right
                        rightMargin: Nheko.paddingSmall
                        top: parent.top
                    }
                }
            }

            // sticker grid
            ListView {
                id: gridView

                property int cellHeight: stickerDimPad

                Layout.column: 1
                Layout.preferredHeight: cellHeight * (stickersPerRow + 0.5)
                Layout.preferredWidth: stickersPerRow * stickerDimPad + 20 - Nheko.paddingSmall
                Layout.row: 1
                boundsBehavior: Flickable.StopAtBounds
                clip: true
                currentIndex: -1 // prevent sorting from stealing focus

                model: roomid ? TimelineManager.completerFor(stickerPopup.emoji ? "emojigrid" : "stickergrid", roomid) : null
                section.criteria: ViewSection.FullString
                section.labelPositioning: ViewSection.InlineLabels | ViewSection.CurrentLabelAtStart
                section.property: "packname"
                spacing: Nheko.paddingSmall

                ScrollBar.vertical: ScrollBar {
                    id: emojiScroll

                }

                // Individual emoji
                delegate: Row {
                    required property var row

                    spacing: Nheko.paddingSmall

                    Repeater {
                        model: row

                        delegate: AbstractButton {
                            id: del

                            required property var modelData

                            ToolTip.text: ":" + modelData.shortcode + ": - " + (modelData.unicode ? modelData.unicodeName : modelData.body)
                            ToolTip.visible: hovered
                            height: stickerDim
                            hoverEnabled: true
                            width: stickerDim

                            background: Rectangle {
                                anchors.fill: parent
                                color: hovered ? palette.highlight : 'transparent'
                                radius: 5
                            }
                            contentItem: DelegateChooser {
                                roleValue: del.modelData.unicode != undefined

                                DelegateChoice {
                                    roleValue: true

                                    Text {
                                        font.family: Settings.emojiFont
                                        font.pixelSize: 36
                                        height: stickerDim
                                        horizontalAlignment: Text.AlignHCenter
                                        text: del.modelData.unicode.replace('\ufe0f', '')
                                        verticalAlignment: Text.AlignVCenter
                                        width: stickerDim
                                    }
                                }
                                DelegateChoice {
                                    roleValue: false

                                    Image {
                                        fillMode: Image.PreserveAspectFit
                                        height: stickerDim
                                        source: del.modelData.url.replace("mxc://", "image://MxcImage/") + "?scale"
                                        width: stickerDim
                                    }
                                }
                            }

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
                        }
                    }
                }
                section.delegate: Rectangle {
                    required property string section

                    color: palette.alternateBase
                    height: childrenRect.height
                    width: gridView.width

                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        font.bold: true
                        text: parent.section
                    }
                }
            }
            ListView {
                Layout.column: 0
                Layout.fillHeight: true
                Layout.preferredWidth: sidebarAvatarSize
                Layout.rightMargin: Nheko.paddingSmall
                Layout.row: 1
                clip: true
                model: gridView.model ? gridView.model.sections : null
                spacing: Nheko.paddingSmall

                delegate: Avatar {
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: modelData.name
                    ToolTip.visible: hovered
                    displayName: modelData.name
                    height: sidebarAvatarSize
                    hoverEnabled: true
                    roomid: modelData.name
                    textColor: modelData.url.startsWith("mxc://") ? palette.text : palette.buttonText
                    url: modelData.url.replace("mxc://", "image://MxcImage/")
                    width: sidebarAvatarSize

                    onClicked: gridView.positionViewAtIndex(modelData.firstRowWith, ListView.Beginning)
                }
            }
            ImageButton {
                Layout.column: 0
                Layout.preferredHeight: sidebarAvatarSize
                Layout.preferredWidth: sidebarAvatarSize
                Layout.rightMargin: Nheko.paddingSmall
                Layout.row: 0
                ToolTip.delay: Nheko.tooltipDelay
                ToolTip.text: qsTr("Change what packs are enabled, remove packs, or create new ones")
                ToolTip.visible: hovered
                hoverEnabled: true
                image: ":/icons/icons/ui/settings.svg"

                onClicked: TimelineManager.openImagePackSettings(stickerPopup.roomid)
            }
        }
    }
}
