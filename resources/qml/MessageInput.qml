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
            Layout.alignment: Qt.AlignBottom
            hoverEnabled: true
            width: 22
            height: 22
            image: ":/icons/icons/ui/place-call.png"
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            Layout.leftMargin: 16
        }

        ImageButton {
            Layout.alignment: Qt.AlignBottom
            hoverEnabled: true
            width: 22
            height: 22
            image: ":/icons/icons/ui/paper-clip-outline.png"
            Layout.topMargin: 8
            Layout.bottomMargin: 8
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

                Connections {
                    target: TimelineManager.timeline.input
                    function onInsertText(text_) { textArea.insert(textArea.cursorPosition, text_); }
                }

                Keys.onPressed: {
                    if (event.matches(StandardKey.Paste)) {
                        TimelineManager.timeline.input.paste(false)
                        event.accepted = true
                    }
                    else if (event.matches(StandardKey.InsertParagraphSeparator)) {
                        TimelineManager.timeline.input.send()
                        textArea.clear()
                        event.accepted = true
                    }
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
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            hoverEnabled: true
            width: 22
            height: 22
            image: ":/icons/icons/ui/smile.png"
            Layout.topMargin: 8
            Layout.bottomMargin: 8
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
        }

    }

}
