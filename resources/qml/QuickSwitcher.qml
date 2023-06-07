// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

Popup {
    id: quickSwitcher

    property int textHeight: Math.round(Qt.application.font.pixelSize * 2.4)
    property int textMargin: Nheko.paddingSmall

    background: null
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    modal: true

    // Workaround palettes not inheriting for popups
    palette: timelineRoot.palette
    parent: Overlay.overlay
    width: Math.min(Math.max(Math.round(parent.width / 2), 450), parent.width) // limiting width to parent.width/2 can be a bit narrow
    x: Math.round(parent.width / 2 - contentWidth / 2)
    y: Math.round(parent.height / 4)

    Overlay.modal: Rectangle {
        color: "#aa1E1E1E"
    }

    onClosed: TimelineManager.focusMessageInput()
    onOpened: {
        roomTextInput.forceActiveFocus();
    }

    Column {
        anchors.fill: parent
        spacing: 1

        MatrixTextField {
            id: roomTextInput

            color: palette.text
            font.pixelSize: Math.ceil(quickSwitcher.textHeight * 0.6)
            width: parent.width

            Keys.onPressed: event => {
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

            avatarHeight: quickSwitcher.textHeight
            avatarWidth: quickSwitcher.textHeight
            bottomToTop: false
            centerRowContent: false
            completerName: "room"
            fullWidth: true
            rowMargin: Math.round(quickSwitcher.textMargin / 2)
            rowSpacing: quickSwitcher.textMargin
            visible: roomTextInput.text.length > 0
            width: parent.width
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
}
