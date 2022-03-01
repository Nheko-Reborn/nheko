// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

Popup {
    id: quickSwitcher

    property int textHeight: Math.round(Qt.application.font.pixelSize * 2.4)

    background: null
    width: Math.min(Math.max(Math.round(parent.width / 2),450),parent.width) // limiting width to parent.width/2 can be a bit narrow
    x: Math.round(parent.width / 2 - contentWidth / 2)
    y: Math.round(parent.height / 4)
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    parent: Overlay.overlay
    palette: Nheko.colors
    onOpened: {
        roomTextInput.forceActiveFocus();
    }
    property int textMargin: Nheko.paddingSmall

    Column{
        anchors.fill: parent
        spacing: 1

        MatrixTextField {
            id: roomTextInput

            width: parent.width
            font.pixelSize: Math.ceil(quickSwitcher.textHeight * 0.6)
            color: Nheko.colors.text
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

            visible: roomTextInput.text.length > 0
            width: parent.width
            completerName: "room"
            bottomToTop: false
            fullWidth: true
            avatarHeight: quickSwitcher.textHeight
            avatarWidth: quickSwitcher.textHeight
            centerRowContent: false
            rowMargin: Math.round(quickSwitcher.textMargin / 2)
            rowSpacing: quickSwitcher.textMargin
        }
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
