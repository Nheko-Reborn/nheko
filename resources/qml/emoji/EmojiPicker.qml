import QtQuick 2.9
import QtQuick.Controls 2.9
import QtQuick.Layouts 1.3

import im.nheko 1.0
import im.nheko.EmojiModel 1.0

import "../"

Popup {
    property var colors
    property alias model: gridView.model
    property var textArea
    property string emojiCategory: "people"

    id: emojiPopup

    margins: 0

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    ColumnLayout {
        anchors.fill: parent

        // Search field
        TextField {
            id: emojiSearch
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: parent.width - 4
            visible: emojiPopup.model.category === Emoji.Category.Search
            placeholderText: qsTr("Search")
            selectByMouse: true
            rightPadding: clearSearch.width

            Timer {
                id: searchTimer
                interval: 350 // tweak as needed?
                onTriggered: emojiPopup.model.filter = emojiSearch.text
            }

            ToolButton {
                id: clearSearch
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                }
                // clear the default hover effects.
                background: Item {}
                visible: emojiSearch.text !== ''
                icon.source: "image://colorimage/:/icons/icons/ui/round-remove-button.png?" + (clearSearch.hovered ? colors.highlight : colors.buttonText)
                focusPolicy: Qt.NoFocus
                onClicked: emojiSearch.clear()
            }

            onTextChanged: searchTimer.restart()
            onVisibleChanged: if (visible) forceActiveFocus()
        }

        // emoji grid
        GridView {
            id: gridView

            Layout.fillWidth: true
            Layout.fillHeight: true

            cellWidth: 52
            cellHeight: 52

            boundsBehavior: Flickable.DragOverBounds

            clip: true

            // Individual emoji
            delegate: AbstractButton {
                width: 48
                height: 48

                contentItem: Text {
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    font.pointSize: 36
                    text: model.unicode
                }

                background: Rectangle {
                    anchors.fill: parent
                    color: hovered ? colors.highlight : 'transparent'
                    radius: 5
                }

                hoverEnabled: true
                ToolTip.text: model.shortName
                ToolTip.visible: hovered
                // TODO: emit a signal and maybe add favorites at some point?
                //onClicked: textArea.insert(textArea.cursorPosition, modelData.unicode)
            }

            ScrollBar.vertical: ScrollBar {}
        }

        // Separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 2

            color: emojiPopup.colors.highlight
        }

        // Category picker row
        Row {
            Repeater {
                model: ListModel {
                    // TODO: Would like to get 'simple' icons for the categories
                    ListElement { label: "😏"; category: Emoji.Category.People }
                    ListElement { label: "🌲"; category: Emoji.Category.Nature }
                    ListElement { label: "🍛"; category: Emoji.Category.Food }
                    ListElement { label: "🚁"; category: Emoji.Category.Activity }
                    ListElement { label: "🚅"; category: Emoji.Category.Travel }
                    ListElement { label: "💡"; category: Emoji.Category.Objects }
                    ListElement { label: "🔣"; category: Emoji.Category.Symbols }
                    ListElement { label: "🏁"; category: Emoji.Category.Flags }
                    ListElement { label: "🔍"; category: Emoji.Category.Search }
                }

                delegate: AbstractButton {
                    width: 40
                    height: 40

                    contentItem: Text {
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        font.pointSize: 30 
                        text: model.label
                    }

                    background: Rectangle {
                        anchors.fill: parent
                        color: emojiPopup.model.category === model.category ? colors.highlight : 'transparent'
                        radius: 5
                    }

                    hoverEnabled: true
					ToolTip.text: {
						switch (model.category) {
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
						case Emoji.Category.Search:
							return qsTr('Search');
						}
					}
                    ToolTip.visible: hovered

                    onClicked: emojiPopup.model.category = model.category
                }
            }
        }
    }
}
