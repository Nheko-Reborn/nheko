// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates/"
import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko 1.0

Popup {
    id: forwardMessagePopup

    property string mid: ""

    function setMessageEventId(mid_in) {
        mid = mid_in;
    }

    leftPadding: 10
    modal: true

    // Workaround palettes not inheriting for popups
    palette: timelineRoot.palette
    parent: Overlay.overlay
    rightPadding: 10
    width: timelineRoot.width * 0.8
    x: Math.round(parent.width / 2 - width / 2)
    y: Math.round(parent.height / 4)

    Overlay.modal: Rectangle {
        color: Qt.rgba(palette.window.r, palette.window.g, palette.window.b, 0.7)
    }
    background: Rectangle {
        color: palette.window
    }

    onOpened: {
        roomTextInput.forceActiveFocus();
    }

    Column {
        id: forwardColumn

        spacing: 5

        Label {
            id: titleLabel

            bottomPadding: 10
            color: palette.text
            font.bold: true
            text: qsTr("Forward Message")
        }
        Reply {
            id: replyPreview

            eventId: mid
            userColor: TimelineManager.userColor(replyPreview.userId, palette.window)
            maxWidth: parent.width
        }
        MatrixTextField {
            id: roomTextInput

            color: palette.text
            width: forwardMessagePopup.width - forwardMessagePopup.leftPadding * 2

            Keys.onPressed: (event) => {
                if (event.key == Qt.Key_Up || event.key == Qt.Key_Backtab) {
                    event.accepted = true;
                    completerPopup.up();
                } else if (event.key == Qt.Key_Down || event.key == Qt.Key_Tab) {
                    event.accepted = true;
                    if (event.key == Qt.Key_Tab && (event.modifiers & Qt.ShiftModifier))
                        completerPopup.up();
                    else
                        completerPopup.down();
                } else if (event.matches(StandardKey.InsertParagraphSeparator)) {
                    completerPopup.finishCompletion();
                    event.accepted = true;
                }
            }
            onTextEdited: {
                completerPopup.completer.searchString = text;
            }
        }
        Completer {
            id: completerPopup

            avatarHeight: 24
            avatarWidth: 24
            bottomToTop: false
            centerRowContent: false
            completerName: "room"
            fullWidth: true
            width: forwardMessagePopup.width - forwardMessagePopup.leftPadding * 2
        }
    }
    Connections {
        function onCompletionSelected(id) {
            room.forwardMessage(forwardMessagePopup.mid, id);
            forwardMessagePopup.close();
        }
        function onCountChanged() {
            if (completerPopup.count > 0 && (completerPopup.currentIndex < 0 || completerPopup.currentIndex >= completerPopup.count))
                completerPopup.currentIndex = 0;
        }

        target: completerPopup
    }
}
