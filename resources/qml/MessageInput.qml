import "./voip"
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

    Component {
        id: placeCallDialog

        PlaceCall {
        }

    }

    RowLayout {
        id: inputBar

        anchors.fill: parent
        spacing: 16

        ImageButton {
            visible: CallManager.callsSupported
            opacity: CallManager.haveCallInvite ? 0.3 : 1
            Layout.alignment: Qt.AlignBottom
            hoverEnabled: true
            width: 22
            height: 22
            image: CallManager.isOnCall ? ":/icons/icons/ui/end-call.png" : ":/icons/icons/ui/place-call.png"
            ToolTip.visible: hovered
            ToolTip.text: CallManager.isOnCall ? qsTr("Hang up") : qsTr("Place a call")
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            Layout.leftMargin: 16
            onClicked: {
                if (TimelineManager.timeline) {
                    if (CallManager.haveCallInvite) {
                        return ;
                    } else if (CallManager.isOnCall) {
                        CallManager.hangUp();
                    } else {
                        CallManager.refreshDevices();
                        var dialog = placeCallDialog.createObject(timelineRoot);
                        dialog.open();
                    }
                }
            }
        }

        ImageButton {
            Layout.alignment: Qt.AlignBottom
            hoverEnabled: true
            width: 22
            height: 22
            image: ":/icons/icons/ui/paper-clip-outline.png"
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            Layout.leftMargin: CallManager.callsSupported ? 0 : 16
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

                property int completerTriggeredAt: -1

                function insertCompletion(completion) {
                    textArea.remove(completerTriggeredAt, cursorPosition);
                    textArea.insert(cursorPosition, completion);
                }

                function openCompleter(pos, type) {
                    completerTriggeredAt = pos;
                    popup.completerName = type;
                    popup.open();
                    popup.completer.setSearchString(textArea.getText(completerTriggeredAt, cursorPosition));
                }

                selectByMouse: true
                placeholderText: qsTr("Write a message...")
                placeholderTextColor: colors.buttonText
                color: colors.text
                wrapMode: TextEdit.Wrap
                focus: true
                onTextChanged: TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text)
                onCursorPositionChanged: {
                    TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text);
                    if (cursorPosition <= completerTriggeredAt) {
                        completerTriggeredAt = -1;
                        popup.close();
                    }
                    if (popup.opened)
                        popup.completer.setSearchString(textArea.getText(completerTriggeredAt, cursorPosition));

                }
                onSelectionStartChanged: TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text)
                onSelectionEndChanged: TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text)
                // Ensure that we get escape key press events first.
                Keys.onShortcutOverride: event.accepted = (completerTriggeredAt != -1 && (event.key === Qt.Key_Escape || event.key === Qt.Key_Tab || event.key === Qt.Key_Enter))
                Keys.onPressed: {
                    if (event.matches(StandardKey.Paste)) {
                        TimelineManager.timeline.input.paste(false);
                        event.accepted = true;
                    } else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_U) {
                        textArea.clear();
                    } else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_P) {
                        textArea.text = TimelineManager.timeline.input.previousText();
                    } else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_N) {
                        textArea.text = TimelineManager.timeline.input.nextText();
                    } else if (event.key == Qt.Key_At) {
                        textArea.openCompleter(cursorPosition, "user");
                        popup.open();
                    } else if (event.key == Qt.Key_Colon) {
                        textArea.openCompleter(cursorPosition, "emoji");
                        popup.open();
                    } else if (event.key == Qt.Key_Escape && popup.opened) {
                        completerTriggeredAt = -1;
                        popup.completerName = "";
                        event.accepted = true;
                        popup.close();
                    } else if (event.matches(StandardKey.InsertParagraphSeparator)) {
                        if (popup.opened) {
                            var currentCompletion = popup.currentCompletion();
                            popup.completerName = "";
                            popup.close();
                            if (currentCompletion) {
                                textArea.insertCompletion(currentCompletion);
                                event.accepted = true;
                                return ;
                            }
                        }
                        TimelineManager.timeline.input.send();
                        textArea.clear();
                        event.accepted = true;
                    } else if (event.key == Qt.Key_Tab) {
                        event.accepted = true;
                        if (popup.opened) {
                            popup.up();
                        } else {
                            var pos = cursorPosition - 1;
                            while (pos > -1) {
                                var t = textArea.getText(pos, pos + 1);
                                console.log('"' + t + '"');
                                if (t == '@' || t == ' ' || t == '\t') {
                                    textArea.openCompleter(pos, "user");
                                    return ;
                                } else if (t == ':') {
                                    textArea.openCompleter(pos, "emoji");
                                    return ;
                                }
                                pos = pos - 1;
                            }
                            // At start of input
                            textArea.openCompleter(0, "user");
                        }
                    } else if (event.key == Qt.Key_Up && popup.opened) {
                        event.accepted = true;
                        popup.up();
                    } else if (event.key == Qt.Key_Down && popup.opened) {
                        event.accepted = true;
                        popup.down();
                    }
                }

                Connections {
                    onTimelineChanged: {
                        textArea.clear();
                        textArea.append(TimelineManager.timeline.input.text());
                        textArea.completerTriggeredAt = -1;
                        popup.completerName = "";
                    }
                    target: TimelineManager
                }

                Connections {
                    onCompletionClicked: textArea.insertCompletion(completion)
                    target: popup
                }

                Completer {
                    id: popup

                    x: textArea.positionToRectangle(textArea.completerTriggeredAt).x
                    y: textArea.positionToRectangle(textArea.completerTriggeredAt).y - height
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

                NhekoDropArea {
                    anchors.fill: parent
                    roomid: TimelineManager.timeline.roomId()
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
