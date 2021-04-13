// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko 1.0

Popup {
    id: forwardMessagePopup
    x: 400
    y: 400

    width: 200

    property var mid

    onOpened: {
        completerPopup.open();
        roomTextInput.forceActiveFocus();
    }

    onClosed: {
        completerPopup.close();
    }

    background: Rectangle {
        border.color: "#444"
    }

    function setMessageEventId(mid_in) {
        mid = mid_in;
    }

    MatrixTextField {
        id: roomTextInput

        width: forwardMessagePopup.width - forwardMessagePopup.leftPadding * 2

        color: colors.text
        onTextEdited: {
            completerPopup.completer.searchString = text;
        }
        Keys.onPressed: {
            if (event.key == Qt.Key_Up && completerPopup.opened) {
                event.accepted = true;
                completerPopup.up();
            } else if (event.key == Qt.Key_Down && completerPopup.opened) {
                event.accepted = true;
                completerPopup.down();
            } else if (event.matches(StandardKey.InsertParagraphSeparator)) {
                completerPopup.finishCompletion();
                event.accepted = true;
            }
        }
    }

    Completer {
        id: completerPopup

        y: roomTextInput.height + roomTextInput.bottomPadding
        width: forwardMessagePopup.width - forwardMessagePopup.leftPadding * 2
        completerName: "room"
        avatarHeight: 24
        avatarWidth: 24
        bottomToTop: false
        closePolicy: Popup.NoAutoClose
    }

    Connections {
        onCompletionSelected: {
            TimelineManager.timeline.forwardMessage(messageContextMenu.eventId, id);
            forwardMessagePopup.close();
        }
        onCountChanged: {
            if (completerPopup.count > 0 && (completerPopup.currentIndex < 0 || completerPopup.currentIndex >= completerPopup.count))
                completerPopup.currentIndex = 0;
        }
        target: completerPopup
    }
}