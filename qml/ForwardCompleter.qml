// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates/"
import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko

Popup {
    id: forwardMessagePopup

    property var mid

    function setMessageEventId(mid_in) {
        mid = mid_in;
    }

    x: Math.round(parent.width / 2 - width / 2)
    y: Math.round(parent.height / 4)
    modal: true
    palette: timelineRoot.palette
    parent: Overlay.overlay
    width: timelineRoot.width * 0.8
    leftPadding: 10
    rightPadding: 10
    onOpened: {
        roomTextInput.forceActiveFocus();
    }

    Column {
        id: forwardColumn

        spacing: 5

        Label {
            id: titleLabel

            text: qsTr("Forward Message")
            font.bold: true
            bottomPadding: 10
            color: timelineRoot.palette.text
        }

        Reply {
            id: replyPreview

            property var modelData: room ? room.getDump(mid, "") : {
            }

            width: parent.width

            userColor: TimelineManager.userColor(modelData.userId, timelineRoot.palette.window)
            blurhash: modelData.blurhash ?? ""
            body: modelData.body ?? ""
            formattedBody: modelData.formattedBody ?? ""
            eventId: modelData.eventId ?? ""
            filename: modelData.filename ?? ""
            filesize: modelData.filesize ?? ""
            proportionalHeight: modelData.proportionalHeight ?? 1
            type: modelData.type ?? MtxEvent.UnknownMessage
            typeString: modelData.typeString ?? ""
            url: modelData.url ?? ""
            originalWidth: modelData.originalWidth ?? 0
            isOnlyEmoji: modelData.isOnlyEmoji ?? false
            userId: modelData.userId ?? ""
            userName: modelData.userName ?? ""
            encryptionError: modelData.encryptionError ?? ""
        }

        MatrixTextField {
            id: roomTextInput

            width: forwardMessagePopup.width - forwardMessagePopup.leftPadding * 2
            color: timelineRoot.palette.text
            onTextEdited: {
                completerPopup.completer.searchString = text;
            }
            Keys.onPressed: {
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
        }

        Completer {
            id: completerPopup

            width: forwardMessagePopup.width - forwardMessagePopup.leftPadding * 2
            completerName: "room"
            fullWidth: true
            centerRowContent: false
            avatarHeight: 24
            avatarWidth: 24
            bottomToTop: false
        }

    }

    Connections {
        function onCompletionSelected(id) {
            room.forwardMessage(messageContextMenu.eventId, id);
            forwardMessagePopup.close();
        }

        function onCountChanged() {
            if (completerPopup.count > 0 && (completerPopup.currentIndex < 0 || completerPopup.currentIndex >= completerPopup.count))
                completerPopup.currentIndex = 0;

        }

        target: completerPopup
    }

    background: Rectangle {
        color: timelineRoot.palette.window
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(timelineRoot.palette.window.r, timelineRoot.palette.window.g, timelineRoot.palette.window.b, 0.7)
    }

}
