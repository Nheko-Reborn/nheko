// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko 1.0

Popup {
    id: roomDirectory

    property int textHeight: Math.round(Qt.application.font.pixelSize * 2.4)
    property int textMargin: Math.round(textHeight / 8)

    background: null
    width: Math.round(parent.width / 2)
    x: Math.round(parent.width / 2 - width / 2)
    y: Math.round(parent.height / 4 - height / 2)
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    parent: Overlay.overlay
    palette: colors
    onOpened: {
        console.log("Opened up room dir popup");
    }

    MatrixTextField {
        id: roomTextInput

        anchors.fill: parent
        font.pixelSize: Math.ceil(quickSwitcher.textHeight * 0.6)
        padding: textMargin
        color: colors.text
        onTextEdited: {
        }
        Keys.onPressed: {
        }
    }

    Rectangle {
        anchors.fill: parent

        ListModel {
            id: publicRoomsListModel

            ListElement {
                name: "nheko"
            }

            ListElement {
                name: "construct"
            }
        }   
    }

    onClosed: {
        roomDirectory.close();
    }

    Overlay.modal: Rectangle {
        color: "#aa1E1E1E"
    }
}