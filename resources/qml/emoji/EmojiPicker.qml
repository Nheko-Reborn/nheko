import "../"
import QtGraphicalEffects 1.0
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import im.nheko 1.0
import im.nheko.EmojiModel 1.0

Popup {
    id: emojiPopup

    property string event_id
    property var colors
    property alias model: gridView.model
    property var textArea
    property string emojiCategory: "people"
    property real highlightHue: colors.highlight.hslHue
    property real highlightSat: colors.highlight.hslSaturation
    property real highlightLight: colors.highlight.hslLightness

    function show(showAt, event_id) {
        console.debug("Showing emojiPicker for " + event_id);
        if (showAt) {
            parent = showAt;
            x = Math.round((showAt.width - width) / 2);
            y = showAt.height;
        }
        emojiPopup.event_id = event_id;
        open();
    }

    margins: 0
    bottomPadding: 1
    leftPadding: 1
    rightPadding: 1
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    ColumnLayout {
        id: columnView

        anchors.fill: parent
        spacing: 0
        Layout.bottomMargin: 0
        Layout.leftMargin: 3
        Layout.rightMargin: 3
        Layout.topMargin: 2

        // emoji grid
        GridView {
            id: gridView

            Layout.preferredHeight: emojiPopup.height
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 4
            cellWidth: 52
            cellHeight: 52
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            // Individual emoji
            delegate: AbstractButton {
                width: 48
                height: 48
                hoverEnabled: true
                ToolTip.text: model.shortName
                ToolTip.visible: hovered
                // TODO: maybe add favorites at some point?
                onClicked: {
                    console.debug("Picked " + model.unicode + "in response to " + emojiPopup.event_id);
                    emojiPopup.close();
                    TimelineManager.queueReactionMessage(emojiPopup.event_id, model.unicode);
                }

                // give the emoji a little oomf
                DropShadow {
                    width: parent.width
                    height: parent.height
                    horizontalOffset: 3
                    verticalOffset: 3
                    radius: 8
                    samples: 17
                    color: "#80000000"
                    source: parent.contentItem
                }

                contentItem: Text {
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.family: Settings.emojiFont
                    font.pixelSize: 36
                    text: model.unicode
                }

                background: Rectangle {
                    anchors.fill: parent
                    color: hovered ? colors.highlight : 'transparent'
                    radius: 5
                }

            }

            // Search field
            header: TextField {
                id: emojiSearch

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: emojiScroll.width + 4
                placeholderText: qsTr("Search")
                selectByMouse: true
                rightPadding: clearSearch.width
                onTextChanged: searchTimer.restart()
                onVisibleChanged: {
                    if (visible)
                        forceActiveFocus();

                }

                Timer {
                    id: searchTimer

                    interval: 350 // tweak as needed?
                    onTriggered: {
                        emojiPopup.model.filter = emojiSearch.text;
                        emojiPopup.model.category = EmojiCategory.Search;
                    }
                }

                ToolButton {
                    id: clearSearch

                    visible: emojiSearch.text !== ''
                    icon.source: "image://colorimage/:/icons/icons/ui/round-remove-button.png?" + (clearSearch.hovered ? colors.highlight : colors.buttonText)
                    focusPolicy: Qt.NoFocus
                    onClicked: emojiSearch.clear()

                    anchors {
                        verticalCenter: parent.verticalCenter
                        right: parent.right
                    }
                    // clear the default hover effects.

                    background: Item {
                    }

                }

            }

            ScrollBar.vertical: ScrollBar {
                id: emojiScroll
            }

        }

        // Separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: emojiPopup.colors.dark
        }

        // Category picker row
        RowLayout {
            Layout.bottomMargin: 0
            Layout.preferredHeight: 42
            implicitHeight: 42
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom

            // Display the normal categories
            Repeater {

                model: ListModel {
                    // TODO: Would like to get 'simple' icons for the categories
                    ListElement {
                        image: ":/icons/icons/emoji-categories/people.png"
                        category: EmojiCategory.People
                    }

                    ListElement {
                        image: ":/icons/icons/emoji-categories/nature.png"
                        category: EmojiCategory.Nature
                    }

                    ListElement {
                        image: ":/icons/icons/emoji-categories/foods.png"
                        category: EmojiCategory.Food
                    }

                    ListElement {
                        image: ":/icons/icons/emoji-categories/activity.png"
                        category: EmojiCategory.Activity
                    }

                    ListElement {
                        image: ":/icons/icons/emoji-categories/travel.png"
                        category: EmojiCategory.Travel
                    }

                    ListElement {
                        image: ":/icons/icons/emoji-categories/objects.png"
                        category: EmojiCategory.Objects
                    }

                    ListElement {
                        image: ":/icons/icons/emoji-categories/symbols.png"
                        category: EmojiCategory.Symbols
                    }

                    ListElement {
                        image: ":/icons/icons/emoji-categories/flags.png"
                        category: EmojiCategory.Flags
                    }

                }

                delegate: AbstractButton {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    hoverEnabled: true
                    ToolTip.text: {
                        switch (model.category) {
                        case EmojiCategory.People:
                            return qsTr('People');
                        case EmojiCategory.Nature:
                            return qsTr('Nature');
                        case EmojiCategory.Food:
                            return qsTr('Food');
                        case EmojiCategory.Activity:
                            return qsTr('Activity');
                        case EmojiCategory.Travel:
                            return qsTr('Travel');
                        case EmojiCategory.Objects:
                            return qsTr('Objects');
                        case EmojiCategory.Symbols:
                            return qsTr('Symbols');
                        case EmojiCategory.Flags:
                            return qsTr('Flags');
                        }
                    }
                    ToolTip.visible: hovered
                    onClicked: {
                        emojiPopup.model.category = model.category;
                    }

                    MouseArea {
                        id: mouseArea

                        anchors.fill: parent
                        onPressed: mouse.accepted = false
                        cursorShape: Qt.PointingHandCursor
                    }

                    contentItem: Image {
                        horizontalAlignment: Image.AlignHCenter
                        verticalAlignment: Image.AlignVCenter
                        fillMode: Image.Pad
                        sourceSize.width: 32
                        sourceSize.height: 32
                        source: "image://colorimage/" + model.image + "?" + (hovered ? colors.highlight : colors.buttonText)
                    }

                    background: Rectangle {
                        anchors.fill: parent
                        color: emojiPopup.model.category === model.category ? Qt.hsla(highlightHue, highlightSat, highlightLight, 0.2) : 'transparent'
                        radius: 5
                        border.color: emojiPopup.model.category === model.category ? colors.highlight : 'transparent'
                    }

                }

            }

            // Separator
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                implicitWidth: 1
                height: parent.height
                color: emojiPopup.colors.dark
            }

            // Search Button is special
            AbstractButton {
                id: searchBtn

                hoverEnabled: true
                Layout.alignment: Qt.AlignRight
                Layout.bottomMargin: 0
                ToolTip.text: qsTr("Search")
                ToolTip.visible: hovered
                onClicked: {
                    // clear any filters
                    emojiPopup.model.category = EmojiCategory.Search;
                    gridView.positionViewAtBeginning();
                    emojiSearch.forceActiveFocus();
                }
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                implicitWidth: 36
                implicitHeight: 36

                MouseArea {
                    id: mouseArea

                    anchors.fill: parent
                    onPressed: mouse.accepted = false
                    cursorShape: Qt.PointingHandCursor
                }

                contentItem: Image {
                    anchors.right: parent.right
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    sourceSize.width: 32
                    sourceSize.height: 32
                    fillMode: Image.Pad
                    smooth: true
                    source: "image://colorimage/:/icons/icons/ui/search.png?" + (parent.hovered ? colors.highlight : colors.buttonText)
                }

            }

        }

    }

}
