// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import im.nheko

ApplicationWindow {
    id: rawMessageRoot

    property alias rawMessage: rawMessageView.text

    color: palette.window
    flags: Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 420
    width: 420

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok

        onAccepted: rawMessageRoot.close()
    }

    Shortcut {
        sequence: StandardKey.Cancel

        onActivated: rawMessageRoot.close()
    }
    ScrollView {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        padding: Nheko.paddingMedium

        TextArea {
            id: rawMessageView

            anchors.fill: parent
            color: palette.text
            font: Nheko.monospaceFont()
            readOnly: true
            textFormat: Text.PlainText

            background: Rectangle {
                color: palette.base
            }
        }
    }
}
