import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import im.nheko 1.0

Rectangle {
    color: colors.window
    Layout.fillWidth: true
    Layout.preferredHeight: textInput.height
    Layout.minimumHeight: 40

    RowLayout {
        id: inputBar

        anchors.fill: parent
        spacing: 16

        ImageButton {
            visible: TimelineManager.callsSupported
            Layout.alignment: Qt.AlignBottom
            hoverEnabled: true
            width: 22
            height: 22
            image: TimelineManager.isOnCall ? ":/icons/icons/ui/end-call.png" : ":/icons/icons/ui/place-call.png"
            ToolTip.visible: hovered
            ToolTip.text: TimelineManager.isOnCall ? qsTr("Hang up") : qsTr("Place a call")
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            Layout.leftMargin: 16
            onClicked: TimelineManager.timeline.input.callButton()
        }

        ImageButton {
            Layout.alignment: Qt.AlignBottom
            hoverEnabled: true
            width: 22
            height: 22
            image: ":/icons/icons/ui/paper-clip-outline.png"
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            Layout.leftMargin: TimelineManager.callsSupported ? 0 : 16
            onClicked: TimelineManager.timeline.input.openFileSelection()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Send a file")

            Rectangle {
                anchors.fill: parent
                color: colors.window
                visible: TimelineManager.timeline.input.uploading

                NhekoBusyIndicator {
                    anchors.fill: parent
                    running: parent.visible
                }

            }

        }

        ScrollView {
            id: textInput

            Layout.alignment: Qt.AlignBottom
            Layout.maximumHeight: Window.height / 4
            Layout.fillWidth: true

            TextArea {
                id: textArea

                placeholderText: qsTr("Write a message...")
                placeholderTextColor: colors.buttonText
                color: colors.text
                wrapMode: TextEdit.Wrap
                onTextChanged: TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text)
                onCursorPositionChanged: TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text)
                onSelectionStartChanged: TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text)
                onSelectionEndChanged: TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text)
                Keys.onPressed: {
                    if (event.matches(StandardKey.Paste)) {
                        TimelineManager.timeline.input.paste(false);
                        event.accepted = true;
                    } else if (event.matches(StandardKey.InsertParagraphSeparator)) {
                        TimelineManager.timeline.input.send();
                        textArea.clear();
                        event.accepted = true;
                    } else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_U)
                        textArea.clear();
                    else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_P)
                        textArea.text = TimelineManager.timeline.input.previousText();
                    else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_N)
                        textArea.text = TimelineManager.timeline.input.nextText();
                }

                Connections {
                    onInsertText: textArea.insert(textArea.cursorPosition, text)
                    target: TimelineManager.timeline.input
                }

                MouseArea {
                    // workaround for wrong cursor shape on some platforms
                    anchors.fill: parent
                    acceptedButtons: Qt.MiddleButton
                    cursorShape: Qt.IBeamCursor
                    onClicked: TimelineManager.timeline.input.paste(true)
                }

                background: Rectangle {
                    color: colors.window
                }

            }

        }

        ImageButton {
            id: emojiButton

            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            hoverEnabled: true
            width: 22
            height: 22
            image: ":/icons/icons/ui/smile.png"
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Emoji")
            onClicked: emojiPopup.visible ? emojiPopup.close() : emojiPopup.show(emojiButton, function(emoji) {
                textArea.insert(textArea.cursorPosition, emoji);
            })
        }

        ImageButton {
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            hoverEnabled: true
            width: 22
            height: 22
            image: ":/icons/icons/ui/cursor.png"
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            Layout.rightMargin: 16
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Send")
            onClicked: {
                TimelineManager.timeline.input.send();
                textArea.clear();
            }
        }

    }

}
