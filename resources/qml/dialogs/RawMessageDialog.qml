// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

ApplicationWindow {
    id: rawMessageRoot

    property alias rawMessage: rawMessageView.text

    height: 420
    width: 420
    palette: Nheko.colors
    color: Nheko.colors.window
    flags: Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: rawMessageRoot.close()
    }

    ScrollView {
        anchors.margins: Nheko.paddingMedium
        anchors.fill: parent
        palette: Nheko.colors
        padding: Nheko.paddingMedium

        TextArea {
            id: rawMessageView

            font: Nheko.monospaceFont()
            color: Nheko.colors.text
            readOnly: true
            textFormat: Text.PlainText

            anchors.fill: parent

            background: Rectangle {
                color: Nheko.colors.base
            }

        }

    }

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok
        onAccepted: rawMessageRoot.close()
    }

}
