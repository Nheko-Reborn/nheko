// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import QtQuick.Window 2.15
import im.nheko

Menu {
    id: emojiPopup

    property var callback
    property var colors
    property string emojiCategory: "people"
    property real highlightHue: timelineRoot.palette.highlight.hslHue
    property real highlightLight: timelineRoot.palette.highlight.hslLightness
    property real highlightSat: timelineRoot.palette.highlight.hslSaturation
    property alias model: gridView.model
    property var textArea

    function show(showAt, callback) {
        console.debug("Showing emojiPicker");
        emojiPopup.callback = callback;
        popup(showAt ? showAt : null);
    }

    bottomPadding: 1
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true
    leftPadding: 1
    margins: 0
    modal: true
    rightPadding: 1
    //height: columnView.implicitHeight + 4
    //width: columnView.implicitWidth
    width: 7 * 52 + 20

    Rectangle {
        color: timelineRoot.palette.window
        height: columnView.implicitHeight + 4
        width: 7 * 52 + 20

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
                Layout.preferredWidth: 7 * 52 + 20 - 6
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

                    onTriggered: {
                        emojiPopup.model.searchString = emojiSearch.text;
                        emojiPopup.model.category = Emoji.Category.Search;
                    }
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
                Layout.preferredHeight: cellHeight * 5
                Layout.preferredWidth: 7 * 52 + 20
                boundsBehavior: Flickable.StopAtBounds
                cacheBuffer: 500
                cellHeight: 52
                cellWidth: 52
                clip: true
                currentIndex: -1 // prevent sorting from stealing focus

                ScrollBar.vertical: ScrollBar {
                    id: emojiScroll
                }

                // Individual emoji
                delegate: AbstractButton {
                    ToolTip.text: model.shortName
                    ToolTip.visible: hovered
                    height: 48
                    hoverEnabled: true
                    width: 48

                    background: Rectangle {
                        anchors.fill: parent
                        color: hovered ? timelineRoot.palette.highlight : 'transparent'
                        radius: 5
                    }

                    // give the emoji a little oomf
                    // DropShadow {
                    //     width: parent.width
                    //     height: parent.height
                    //     horizontalOffset: 3
                    //     verticalOffset: 3
                    //     radius: 8
                    //     samples: 17
                    //     color: "#80000000"
                    //     source: parent.contentItem
                    // }
                    contentItem: Text {
                        color: timelineRoot.palette.text
                        font.family: Settings.emojiFont
                        font.pixelSize: 36
                        horizontalAlignment: Text.AlignHCenter
                        text: model.unicode.replace('\ufe0f', '')
                        verticalAlignment: Text.AlignVCenter
                    }

                    // TODO: maybe add favorites at some point?
                    onClicked: {
                        console.debug("Picked " + model.unicode);
                        emojiPopup.close();
                        callback(model.unicode);
                    }
                }
            }

            // Separator
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: emojiPopup.Nheko.theme.separator
                visible: emojiSearch.text === ''
            }

            // Category picker row
            RowLayout {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                Layout.bottomMargin: 0
                Layout.preferredHeight: 42
                implicitHeight: 42
                visible: emojiSearch.text === ''

                // Display the normal categories
                Repeater {
                    model: [
                        // TODO: Would like to get 'simple' icons for the categories
                        {
                            "image": ":/icons/icons/emoji-categories/people.svg",
                            "category": Emoji.Category.People
                        }, {
                            "image": ":/icons/icons/emoji-categories/nature.svg",
                            "category": Emoji.Category.Nature
                        }, {
                            "image": ":/icons/icons/emoji-categories/foods.svg",
                            "category": Emoji.Category.Food
                        }, {
                            "image": ":/icons/icons/emoji-categories/activity.svg",
                            "category": Emoji.Category.Activity
                        }, {
                            "image": ":/icons/icons/emoji-categories/travel.svg",
                            "category": Emoji.Category.Travel
                        }, {
                            "image": ":/icons/icons/emoji-categories/objects.svg",
                            "category": Emoji.Category.Objects
                        }, {
                            "image": ":/icons/icons/emoji-categories/symbols.svg",
                            "category": Emoji.Category.Symbols
                        }, {
                            "image": ":/icons/icons/emoji-categories/flags.svg",
                            "category": Emoji.Category.Flags
                        }]

                    delegate: AbstractButton {
                        Layout.preferredHeight: 36
                        Layout.preferredWidth: 36
                        ToolTip.text: {
                            switch (modelData.category) {
                            case Emoji.Category.People:
                                return qsTr('People');
                            case Emoji.Category.Nature:
                                return qsTr('Nature');
                            case Emoji.Category.Food:
                                return qsTr('Food');
                            case Emoji.Category.Activity:
                                return qsTr('Activity');
                            case Emoji.Category.Travel:
                                return qsTr('Travel');
                            case Emoji.Category.Objects:
                                return qsTr('Objects');
                            case Emoji.Category.Symbols:
                                return qsTr('Symbols');
                            case Emoji.Category.Flags:
                                return qsTr('Flags');
                            }
                        }
                        ToolTip.visible: hovered
                        hoverEnabled: true

                        background: Rectangle {
                            anchors.fill: parent
                            border.color: emojiPopup.model.category === modelData.category ? timelineRoot.palette.highlight : 'transparent'
                            color: emojiPopup.model.category === modelData.category ? Qt.hsla(highlightHue, highlightSat, highlightLight, 0.2) : 'transparent'
                            radius: 5
                        }
                        contentItem: Image {
                            fillMode: Image.Pad
                            height: 32
                            horizontalAlignment: Image.AlignHCenter
                            mipmap: true
                            smooth: true
                            source: "image://colorimage/" + modelData.image + "?" + (hovered ? timelineRoot.palette.highlight : timelineRoot.palette.placeholderText)
                            sourceSize.height: 32 * Screen.devicePixelRatio
                            sourceSize.width: 32 * Screen.devicePixelRatio
                            verticalAlignment: Image.AlignVCenter
                            width: 32
                        }

                        onClicked: {
                            //emojiPopup.model.category = model.category;
                            gridView.positionViewAtIndex(emojiPopup.model.sourceModel.categoryToIndex(modelData.category), GridView.Beginning);
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor

                            onPressed: mouse.accepted = false
                        }
                    }
                }
            }
        }
    }
}
