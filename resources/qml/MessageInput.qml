// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./emoji"
import "./voip"
import "./ui"
import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0

Rectangle {
    id: inputBar

    property bool showAllButtons: width > 450 || (messageInput.length == 0 && !messageInput.inputMethodComposing)
    readonly property string text: messageInput.text

    Layout.fillWidth: true
    Layout.minimumHeight: 40
    Layout.preferredHeight: row.implicitHeight
    color: palette.window

    Component {
        id: placeCallDialog

        PlaceCall {
        }
    }
    Component {
        id: screenShareDialog

        ScreenShare {
        }
    }
    RowLayout {
        id: row

        anchors.fill: parent
        spacing: 0
        visible: room ? room.permissions.canSend(room.isEncrypted ? MtxEvent.Encrypted :  MtxEvent.TextMessage) : false

        ImageButton {
            Layout.alignment: Qt.AlignBottom
            Layout.margins: 8
            ToolTip.text: CallManager.isOnCall ? qsTr("Hang up") : (CallManager.isOnCallOnOtherDevice ? qsTr("Already on a call") : qsTr("Place a call"))
            ToolTip.visible: hovered
            Layout.preferredHeight: 22
            hoverEnabled: true
            image: CallManager.isOnCall ? ":/icons/icons/ui/end-call.svg" : ":/icons/icons/ui/place-call.svg"
            opacity: (CallManager.haveCallInvite || CallManager.isOnCallOnOtherDevice) ? 0.3 : 1
            visible: CallManager.callsSupported && showAllButtons
            Layout.preferredWidth: 22

            onClicked: {
                if (room) {
                    if (CallManager.haveCallInvite) {
                        return;
                    } else if (CallManager.isOnCall) {
                        CallManager.hangUp();
                    } else if (CallManager.isOnCallOnOtherDevice) {
                        return;
                    } else {
                        var dialog = placeCallDialog.createObject(timelineRoot);
                        dialog.open();
                        timelineRoot.destroyOnClose(dialog);
                    }
                }
            }
        }
        ImageButton {
            Layout.alignment: Qt.AlignBottom
            Layout.margins: 8
            ToolTip.text: qsTr("Send a file")
            ToolTip.visible: hovered
            Layout.preferredHeight: 22
            hoverEnabled: true
            image: ":/icons/icons/ui/attach.svg"
            visible: showAllButtons
            Layout.preferredWidth: 22

            onClicked: room.input.openFileSelection()

            Rectangle {
                anchors.fill: parent
                color: palette.window
                visible: room && room.input.uploading

                Spinner {
                    anchors.centerIn: parent
                    height: parent.height / 2
                    running: parent.visible
                }
            }
        }
        ScrollView {
            id: textInput

            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true
            Layout.maximumHeight: Window.height / 4
            Layout.minimumHeight: fontMetrics.lineSpacing
            Layout.preferredHeight: contentHeight
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            contentWidth: availableWidth

            TextArea {
                id: messageInput

                property int completerTriggeredAt: 0
                property string lastChar

                function insertCompletion(completion) {
                    messageInput.remove(completerTriggeredAt, cursorPosition);
                    messageInput.insert(cursorPosition, completion);
                    let userid = completer.currentUserid();
                    if (userid) {
                        room.input.addMention(userid, completion);
                    }
                }
                function openCompleter(pos, type) {
                    if (popup.opened)
                        return;
                    completerTriggeredAt = pos;
                    completer.completerName = type;
                    popup.open();
                    completer.completer.setSearchString(messageInput.getText(completerTriggeredAt, cursorPosition) + messageInput.preeditText);
                }
                function positionCursorAtEnd() {
                    cursorPosition = messageInput.length;
                }
                function positionCursorAtStart() {
                    cursorPosition = 0;
                }

                background: null
                bottomPadding: 8
                color: palette.text
                focus: true
                leftPadding: inputBar.showAllButtons ? 0 : 8
                padding: 0
                placeholderText: qsTr("Write a message...")
                placeholderTextColor: palette.buttonText
                selectByMouse: true
                topPadding: 8
                verticalAlignment: TextEdit.AlignVCenter
                width: textInput.width
                wrapMode: TextEdit.Wrap

                Keys.onPressed: event => {
                    if (event.matches(StandardKey.Paste)) {
                        event.accepted = room.input.tryPasteAttachment(false);
                    } else if (event.key == Qt.Key_Space) {
                        // close popup if user enters space after colon
                        if (cursorPosition == completerTriggeredAt + 1)
                            popup.close();
                        if (popup.opened && completer.count <= 0)
                            popup.close();
                    } else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_U) {
                        messageInput.clear();
                    } else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_P) {
                        messageInput.text = room.input.previousText();
                    } else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_N) {
                        messageInput.text = room.input.nextText();
                    } else if (event.key == Qt.Key_Escape && popup.opened) {
                        completer.completerName = "";
                        popup.close();
                        event.accepted = true;
                    } else if (event.matches(StandardKey.SelectAll) && popup.opened) {
                        completer.completerName = "";
                        popup.close();
                    } else if (event.key == Qt.Key_Enter || event.key == Qt.Key_Return) {
                        // Handling popup takes priority over newline and sending message.
                        if (popup.opened &&
                            (event.modifiers == Qt.NoModifier
                            || event.modifiers == Qt.ShiftModifier
                            || event.modifiers == Qt.ControlModifier)
                        ) {
                            var currentCompletion = completer.currentCompletion();
                            let userid = completer.currentUserid();

                            completer.completerName = "";
                            popup.close();

                            if (currentCompletion) {
                                messageInput.insertCompletion(currentCompletion);
                                if (userid) {
                                    console.log(userid);
                                    room.input.addMention(userid, currentCompletion);
                                }
                            }
                            event.accepted = true;
                        }
                        // Send message Enter key combination event.
                        else if (Settings.sendMessageKey == 0 && event.modifiers == Qt.NoModifier
                              || Settings.sendMessageKey == 1 && event.modifiers == Qt.ShiftModifier
                              || Settings.sendMessageKey == 2 && event.modifiers == Qt.ControlModifier
                        ) {
                            room.input.send();
                            event.accepted = true;
                        }
                        // Add newline Enter key combination event.
                        else if (Settings.sendMessageKey == 0 && event.modifiers == Qt.ShiftModifier
                              || Settings.sendMessageKey == 1 && event.modifiers == Qt.NoModifier
                              || Settings.sendMessageKey == 2 && event.modifiers == Qt.ShiftModifier
                        ) {
                            messageInput.insert(messageInput.cursorPosition, "\n");
                            event.accepted = true;
                        }
                        // Any other Enter key combo is ignored here.
                    } else if (event.key == Qt.Key_Tab && (event.modifiers == Qt.NoModifier || event.modifiers == Qt.ShiftModifier)) {
                        event.accepted = true;
                        if (popup.opened) {
                            if (event.modifiers & Qt.ShiftModifier)
                                completer.down();
                            else
                                completer.up();
                        } else {
                            var pos = cursorPosition - 1;
                            while (pos > -1) {
                                var t = messageInput.getText(pos, pos + 1);
                                console.log('"' + t + '"');
                                if (t == '@') {
                                    messageInput.openCompleter(pos, "user");
                                    return;
                                } else if (t == ' ' || t == '\t') {
                                    messageInput.openCompleter(pos + 1, "user");
                                    return;
                                } else if (t == ':') {
                                    messageInput.openCompleter(pos, "emoji");
                                    return;
                                } else if (t == '~') {
                                    messageInput.openCompleter(pos, "customEmoji");
                                    return;
                                }
                                pos = pos - 1;
                            }
                            // At start of input
                            messageInput.openCompleter(0, "user");
                        }
                    } else if (event.key == Qt.Key_Up && popup.opened) {
                        event.accepted = true;
                        completer.up();
                    } else if ((event.key == Qt.Key_Down || event.key == Qt.Key_Backtab) && popup.opened) {
                        event.accepted = true;
                        completer.down();
                    } else if (event.key == Qt.Key_Up && (event.modifiers == Qt.NoModifier || event.modifiers == Qt.KeypadModifier)) {
                        if (cursorPosition == 0) {
                            event.accepted = true;
                            var idx = room.edit ? room.idToIndex(room.edit) + 1 : 0;
                            while (true) {
                                var id = room.indexToId(idx);
                                if (!id || room.getDump(id, "").isEditable) {
                                    room.edit = id;
                                    cursorPosition = 0;
                                    Qt.callLater(positionCursorAtEnd);
                                    break;
                                }
                                idx++;
                            }
                        } else if (positionAt(0, cursorRectangle.y + cursorRectangle.height / 2) === 0) {
                            event.accepted = true;
                            positionCursorAtStart();
                        }
                    } else if (event.key == Qt.Key_Down && (event.modifiers == Qt.NoModifier || event.modifiers == Qt.KeypadModifier)) {
                        if (cursorPosition == messageInput.length && room.edit) {
                            event.accepted = true;
                            var idx = room.idToIndex(room.edit) - 1;
                            while (true) {
                                var id = room.indexToId(idx);
                                if (!id || room.getDump(id, "").isEditable) {
                                    room.edit = id;
                                    Qt.callLater(positionCursorAtStart);
                                    break;
                                }
                                idx--;
                            }
                        } else if (positionAt(width, cursorRectangle.y + cursorRectangle.height / 2) === messageInput.length) {
                            event.accepted = true;
                            positionCursorAtEnd();
                        }
                    }
                }
                // Ensure that we get escape key press events first.
                Keys.onShortcutOverride: event => event.accepted = (popup.opened && (event.key === Qt.Key_Escape || event.key === Qt.Key_Tab || event.key === Qt.Key_Enter || event.key === Qt.Key_Space))
                onCursorPositionChanged: {
                    if (!room)
                        return;
                    room.input.updateState(selectionStart, selectionEnd, cursorPosition, text);
                    if (popup.opened && cursorPosition <= completerTriggeredAt)
                        popup.close();
                    if (popup.opened)
                        completer.completer.setSearchString(messageInput.getText(completerTriggeredAt, cursorPosition) + messageInput.preeditText);
                }
                onPreeditTextChanged: {
                    if (popup.opened)
                        completer.completer.setSearchString(messageInput.getText(completerTriggeredAt, cursorPosition) + messageInput.preeditText);
                }
                onSelectionEndChanged: room.input.updateState(selectionStart, selectionEnd, cursorPosition, text)
                onSelectionStartChanged: room.input.updateState(selectionStart, selectionEnd, cursorPosition, text)
                onTextChanged: {
                    if (room)
                        room.input.updateState(selectionStart, selectionEnd, cursorPosition, text);
                    forceActiveFocus();
                    if (cursorPosition > 0)
                        lastChar = text.charAt(cursorPosition - 1);
                    else
                        lastChar = '';
                    if (lastChar == '@') {
                        messageInput.openCompleter(selectionStart - 1, "user");
                    } else if (lastChar == ':') {
                        messageInput.openCompleter(selectionStart - 1, "emoji");
                    } else if (lastChar == '#') {
                        messageInput.openCompleter(selectionStart - 1, "roomAliases");
                    } else if (lastChar == "/" && cursorPosition == 1) {
                        messageInput.openCompleter(selectionStart - 1, "command");
                    }
                }

                Connections {
                    function onRoomChanged() {
                        messageInput.clear();
                        if (room)
                            messageInput.append(room.input.text);
                        completer.completerName = "";
                        messageInput.forceActiveFocus();
                    }

                    target: timelineView
                }
                Connections {
                    function onCompletionClicked(completion) {
                        messageInput.insertCompletion(completion);
                    }

                    target: completer
                }
                Popup {
                    id: popup

                    background: null
                    padding: 0
                    x: messageInput.positionToRectangle(messageInput.completerTriggeredAt).x
                    y: messageInput.positionToRectangle(messageInput.completerTriggeredAt).y - height

                    enter: Transition {
                        NumberAnimation {
                            duration: 100
                            from: 0
                            property: "opacity"
                            to: 1
                        }
                    }
                    exit: Transition {
                        NumberAnimation {
                            duration: 100
                            from: 1
                            property: "opacity"
                            to: 0
                        }
                    }

                    contentItem: Completer {
                        id: completer

                        rowMargin: 2
                        rowSpacing: 0
                    }
                }
                Connections {
                    function onTextChanged(newText) {
                        messageInput.text = newText;
                        messageInput.cursorPosition = newText.length;
                    }

                    ignoreUnknownSignals: true
                    target: room ? room.input : null
                }
                Connections {
                    function onEditChanged() {
                        messageInput.forceActiveFocus();
                    }
                    function onReplyChanged() {
                        messageInput.forceActiveFocus();
                    }
                    function onThreadChanged() {
                        messageInput.forceActiveFocus();
                    }

                    ignoreUnknownSignals: true
                    target: room
                }
                Connections {
                    function onFocusInput() {
                        messageInput.forceActiveFocus();
                    }

                    target: TimelineManager
                }
                MouseArea {
                    acceptedButtons: Qt.MiddleButton
                    // workaround for wrong cursor shape on some platforms
                    anchors.fill: parent
                    cursorShape: Qt.IBeamCursor

                    onPressed: mouse => mouse.accepted = room.input.tryPasteAttachment(true)
                }
            }
        }
        ImageButton {
            id: stickerButton

            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            Layout.margins: 8
            ToolTip.text: qsTr("Stickers")
            ToolTip.visible: hovered
            Layout.preferredHeight: 22
            hoverEnabled: true
            image: ":/icons/icons/ui/sticky-note-solid.svg"
            visible: showAllButtons
            Layout.preferredWidth: 22

            onClicked: stickerPopup.visible ? stickerPopup.close() : stickerPopup.show(stickerButton, room.roomId, function (row) {
                    room.input.sticker(row);
                    TimelineManager.focusMessageInput();
                })

            StickerPicker {
                id: stickerPopup

                emoji: false
            }
        }
        ImageButton {
            id: emojiButton

            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            Layout.margins: 8
            ToolTip.text: qsTr("Emoji")
            ToolTip.visible: hovered
            Layout.preferredHeight: 22
            hoverEnabled: true
            image: ":/icons/icons/ui/smile.svg"
            Layout.preferredWidth: 22

            onClicked: emojiPopup.visible ? emojiPopup.close() : emojiPopup.show(emojiButton, room.roomId, function (plaintext, markdown) {
                    messageInput.insert(messageInput.cursorPosition, markdown);
                    TimelineManager.focusMessageInput();
                })

            StickerPicker {
                id: emojiPopup

                emoji: true
            }
        }
        ImageButton {
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            Layout.margins: 8
            Layout.rightMargin: 8
            ToolTip.text: qsTr("Send")
            ToolTip.visible: hovered
            Layout.preferredHeight: 22
            hoverEnabled: true
            image: ":/icons/icons/ui/send.svg"
            Layout.preferredWidth: 22

            onClicked: {
                room.input.send();
            }
        }
    }
    Label {
        anchors.centerIn: parent
        color: palette.placeholderText
        text: qsTr("You don't have permission to send messages in this room")
        visible: room ? (!room.permissions.canSend(MtxEvent.TextMessage)) : false
    }
}
