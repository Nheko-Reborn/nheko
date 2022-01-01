// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko 1.0

Popup {
    id: quickSwitcher

    property int textHeight: Math.round(Qt.application.font.pixelSize * 2.4)
    property int textMargin: Math.round(textHeight / 8)

    background: null
    width: Math.round(parent.width / 2)
    x: Math.round(parent.width / 2 - width / 2)
    y: Math.round(parent.height / 4 - height / 2)
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    parent: Overlay.overlay
    palette: Nheko.colors
    onOpened: {
        completerPopup.open();
        roomTextInput.forceActiveFocus();
    }
    onClosed: {
        completerPopup.close();
    }

    MatrixTextField {
        id: roomTextInput

        anchors.fill: parent
        font.pixelSize: Math.ceil(quickSwitcher.textHeight * 0.6)
        padding: textMargin
        color: Nheko.colors.text
        onTextEdited: {
            completerPopup.completer.searchString = text;
        }
        Keys.onPressed: {
            if ((event.key == Qt.Key_Up || event.key == Qt.Key_Backtab) && completerPopup.opened) {
                event.accepted = true;
                completerPopup.up();
            } else if ((event.key == Qt.Key_Down || event.key == Qt.Key_Tab) && completerPopup.opened) {
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

        x: roomTextInput.x
        y: roomTextInput.y + quickSwitcher.textHeight + quickSwitcher.textMargin
        visible: roomTextInput.length > 0
        width: parent.width
        completerName: "room"
        bottomToTop: false
        fullWidth: true
        avatarHeight: quickSwitcher.textHeight
        avatarWidth: quickSwitcher.textHeight
        centerRowContent: false
        rowMargin: Math.round(quickSwitcher.textMargin / 2)
        rowSpacing: quickSwitcher.textMargin
        closePolicy: Popup.NoAutoClose
    }

    Connections {
        function onCompletionSelected(id) {
            Rooms.setCurrentRoom(id);
            quickSwitcher.close();
        }

        function onCountChanged() {
            if (completerPopup.count > 0 && (completerPopup.currentIndex < 0 || completerPopup.currentIndex >= completerPopup.count))
                completerPopup.currentIndex = 0;

        }

        target: completerPopup
    }

    Overlay.modal: Rectangle {
        color: "#aa1E1E1E"
    }

}
