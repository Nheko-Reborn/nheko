// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

ApplicationWindow {
    id: rawMessageRoot

    property alias rawMessage: rawMessageView.text

    x: MainWindow.x + (MainWindow.width / 2) - (width / 2)
    y: MainWindow.y + (MainWindow.height / 2) - (height / 2)
    height: 420
    width: 420
    palette: Nheko.colors
    color: Nheko.colors.window
    flags: Qt.Tool | Qt.WindowStaysOnTopHint

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: rawMessageRoot.close()
    }

    ScrollView {
        anchors.fill: parent
        palette: Nheko.colors
        padding: Nheko.paddingMedium

        TextArea {
            id: rawMessageView

            font: Nheko.monospaceFont()
            palette: Nheko.colors
            readOnly: true
        }

    }

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok
        onAccepted: rawMessageRoot.close()
    }
}
