// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates/"
import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko 1.0

Popup {
    id: forwardMessagePopup

    property var mid

    function setMessageEventId(mid_in) {
        mid = mid_in;
    }

    x: Math.round(parent.width / 2 - width / 2)
    y: Math.round(parent.height / 2 - height / 2)
    modal: true
    palette: colors
    parent: Overlay.overlay
    width: implicitWidth >= (timelineRoot.width * 0.8) ? implicitWidth : (timelineRoot.width * 0.8)
    height: implicitHeight + completerPopup.height + padding * 2
    leftPadding: 10
    rightPadding: 10
    onOpened: {
        completerPopup.open();
        roomTextInput.forceActiveFocus();
    }
    onClosed: {
        completerPopup.close();
    }

    Column {
        id: forwardColumn

        spacing: 5

        Label {
            id: titleLabel

            text: qsTr("Forward Message")
            font.bold: true
            bottomPadding: 10
            color: colors.text
        }

        Reply {
            id: replyPreview

            modelData: TimelineManager.timeline ? TimelineManager.timeline.getDump(mid, "") : {
            }
            userColor: TimelineManager.userColor(modelData.userId, colors.window)
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

    }

    Completer {
        id: completerPopup

        y: titleLabel.height + replyPreview.height + roomTextInput.height + roomTextInput.bottomPadding + forwardColumn.spacing * 3
        width: forwardMessagePopup.width - forwardMessagePopup.leftPadding * 2
        completerName: "room"
        fullWidth: true
        centerRowContent: false
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

    background: Rectangle {
        color: colors.window
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(colors.window.r, colors.window.g, colors.window.b, 0.7)
    }

}
