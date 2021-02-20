import "./voip"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import im.nheko 1.0

Rectangle {
    color: colors.window
    Layout.fillWidth: true
    Layout.preferredHeight: textInput.height + 16
    Layout.minimumHeight: 40

    Component {
        id: placeCallDialog

        PlaceCall {
        }

    }

    RowLayout {
        id: inputBar

        anchors.fill: parent
        anchors.margins: 8
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
            Layout.leftMargin: 8
            onClicked: {
                if (TimelineManager.timeline) {
                    if (CallManager.haveCallInvite) {
                        return ;
                    } else if (CallManager.isOnCall) {
                        CallManager.hangUp();
                    } else {
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
            Layout.leftMargin: CallManager.callsSupported ? 0 : 8
            onClicked: TimelineManager.timeline.input.openFileSelection()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Send a file")

            Rectangle {
                anchors.fill: parent
                color: colors.window
                visible: TimelineManager.timeline && TimelineManager.timeline.input.uploading

                NhekoBusyIndicator {
                    anchors.fill: parent
                    running: parent.visible
                }

            }

        }

        Flickable {
            id: textInput

            function ensureVisible(r) {
                if (contentX >= r.x)
                    contentX = r.x;
                else if (contentX + width <= r.x + r.width)
                    contentX = r.x + r.width - width;
                if (contentY >= r.y)
                    contentY = r.y;
                else if (contentY + height <= r.y + r.height)
                    contentY = r.y + r.height - height;
            }

            Layout.alignment: Qt.AlignBottom
            Layout.maximumHeight: Window.height / 4
            Layout.minimumHeight: Settings.fontSize
            Layout.fillWidth: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            implicitWidth: messageInput.width
            implicitHeight: messageInput.height
            contentWidth: messageInput.width
            contentHeight: messageInput.height

            TextArea {
                id: messageInput

                property int completerTriggeredAt: -1

                function insertCompletion(completion) {
                    messageInput.remove(completerTriggeredAt, cursorPosition);
                    messageInput.insert(cursorPosition, completion);
                }

                function openCompleter(pos, type) {
                    completerTriggeredAt = pos;
                    popup.completerName = type;
                    popup.open();
                    popup.completer.setSearchString(messageInput.getText(completerTriggeredAt, cursorPosition));
                }

                selectByMouse: true
                placeholderText: qsTr("Write a message...")
                //placeholderTextColor: colors.buttonText
                // only set the anchors on Qt 5.12 or higher
                // see https://doc.qt.io/qt-5/qml-qtquick-controls2-popup.html#anchors.centerIn-prop
                Component.onCompleted: {
                    if (placeholderTextColor !== undefined)
                        placeholderTextColor = colors.buttonText;

                }
                color: colors.text
                width: textInput.width
                wrapMode: TextEdit.Wrap
                padding: 0
                focus: true
                onTextChanged: {
                    if (TimelineManager.timeline)
                        TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text);

                    forceActiveFocus();
                }
                onCursorRectangleChanged: textInput.ensureVisible(cursorRectangle)
                onCursorPositionChanged: {
                    if (!TimelineManager.timeline)
                        return ;

                    TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text);
                    if (cursorPosition <= completerTriggeredAt) {
                        completerTriggeredAt = -1;
                        popup.close();
                    }
                    if (popup.opened)
                        popup.completer.setSearchString(messageInput.getText(completerTriggeredAt, cursorPosition));

                }
                onSelectionStartChanged: TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text)
                onSelectionEndChanged: TimelineManager.timeline.input.updateState(selectionStart, selectionEnd, cursorPosition, text)
                // Ensure that we get escape key press events first.
                Keys.onShortcutOverride: event.accepted = (completerTriggeredAt != -1 && (event.key === Qt.Key_Escape || event.key === Qt.Key_Tab || event.key === Qt.Key_Enter))
                Keys.onPressed: {
                    if (event.matches(StandardKey.Paste)) {
                        TimelineManager.timeline.input.paste(false);
                        event.accepted = true;
                    } else if (event.key == Qt.Key_Space) {
                        // close popup if user enters space after colon
                        if (cursorPosition == completerTriggeredAt + 1)
                            popup.close();

                        if (popup.opened && popup.count <= 0)
                            popup.close();

                    } else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_U) {
                        messageInput.clear();
                    } else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_P) {
                        messageInput.text = TimelineManager.timeline.input.previousText();
                    } else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_N) {
                        messageInput.text = TimelineManager.timeline.input.nextText();
                    } else if (event.key == Qt.Key_At) {
                        messageInput.openCompleter(cursorPosition, "user");
                        popup.open();
                    } else if (event.key == Qt.Key_Colon) {
                        messageInput.openCompleter(cursorPosition, "emoji");
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
                                messageInput.insertCompletion(currentCompletion);
                                event.accepted = true;
                                return ;
                            }
                        }
                        TimelineManager.timeline.input.send();
                        messageInput.clear();
                        event.accepted = true;
                    } else if (event.key == Qt.Key_Tab) {
                        event.accepted = true;
                        if (popup.opened) {
                            popup.up();
                        } else {
                            var pos = cursorPosition - 1;
                            while (pos > -1) {
                                var t = messageInput.getText(pos, pos + 1);
                                console.log('"' + t + '"');
                                if (t == '@') {
                                    messageInput.openCompleter(pos, "user");
                                    return ;
                                } else if (t == ' ' || t == '\t') {
                                    messageInput.openCompleter(pos + 1, "user");
                                    return ;
                                } else if (t == ':') {
                                    messageInput.openCompleter(pos, "emoji");
                                    return ;
                                }
                                pos = pos - 1;
                            }
                            // At start of input
                            messageInput.openCompleter(0, "user");
                        }
                    } else if (event.key == Qt.Key_Up && popup.opened) {
                        event.accepted = true;
                        popup.up();
                    } else if (event.key == Qt.Key_Down && popup.opened) {
                        event.accepted = true;
                        popup.down();
                    }
                }
                background: null

                Connections {
                    onActiveTimelineChanged: {
                        messageInput.clear();
                        messageInput.append(TimelineManager.timeline.input.text());
                        messageInput.completerTriggeredAt = -1;
                        popup.completerName = "";
                        messageInput.forceActiveFocus();
                    }
                    target: TimelineManager
                }

                Connections {
                    onCompletionClicked: messageInput.insertCompletion(completion)
                    target: popup
                }

                Completer {
                    id: popup

                    x: messageInput.completerTriggeredAt >= 0 ? messageInput.positionToRectangle(messageInput.completerTriggeredAt).x : 0
                    y: messageInput.completerTriggeredAt >= 0 ? messageInput.positionToRectangle(messageInput.completerTriggeredAt).y - height : 0
                }

                Connections {
                    ignoreUnknownSignals: true
                    onInsertText: {
                        messageInput.insert(messageInput.cursorPosition, text);
                    }
                    onTextChanged: {
                        messageInput.text = newText;
                        messageInput.cursorPosition = newText.length;
                    }
                    target: TimelineManager.timeline ? TimelineManager.timeline.input : null
                }

                Connections {
                    ignoreUnknownSignals: true
                    onReplyChanged: messageInput.forceActiveFocus()
                    onEditChanged: messageInput.forceActiveFocus()
                    target: TimelineManager.timeline
                }

                Connections {
                    target: TimelineManager
                    onFocusInput: messageInput.forceActiveFocus()
                }

                MouseArea {
                    // workaround for wrong cursor shape on some platforms
                    anchors.fill: parent
                    acceptedButtons: Qt.MiddleButton
                    cursorShape: Qt.IBeamCursor
                    onClicked: TimelineManager.timeline.input.paste(true)
                }

            }

            ScrollBar.vertical: ScrollBar {
            }

        }

        ImageButton {
            id: emojiButton

            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            hoverEnabled: true
            width: 22
            height: 22
            image: ":/icons/icons/ui/smile.png"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Emoji")
            onClicked: emojiPopup.visible ? emojiPopup.close() : emojiPopup.show(emojiButton, function(emoji) {
                messageInput.insert(messageInput.cursorPosition, emoji);
                TimelineManager.focusMessageInput();
            })
        }

        ImageButton {
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            hoverEnabled: true
            width: 22
            height: 22
            image: ":/icons/icons/ui/cursor.png"
            Layout.rightMargin: 8
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Send")
            onClicked: {
                TimelineManager.timeline.input.send();
                messageInput.clear();
            }
        }

    }

}
