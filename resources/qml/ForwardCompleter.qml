// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates/"
import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko 1.0

Dialog {
    id: forwardMessagePopup
    title: qsTr("Forward Message")
    x: 400
    y: 400

    width: 200
    height: replyPreview.height + roomTextInput.height + completerPopup.height + implicitFooterHeight + implicitHeaderHeight

    property var mid

    onOpened: {
        completerPopup.open();
        roomTextInput.forceActiveFocus();
    }

    onClosed: {
        completerPopup.close();
    }

    function setMessageEventId(mid_in) {
        mid = mid_in;
    }

    Reply {
        id: replyPreview
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        modelData: TimelineManager.timeline ? TimelineManager.timeline.getDump(mid, "") : {
        }
        userColor: TimelineManager.userColor(modelData.userId, colors.window)
    }

    MatrixTextField {
        id: roomTextInput

        width: forwardMessagePopup.width - forwardMessagePopup.leftPadding * 2

        anchors.top: replyPreview.bottom

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

        y: replyPreview.height + roomTextInput.height + roomTextInput.bottomPadding

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