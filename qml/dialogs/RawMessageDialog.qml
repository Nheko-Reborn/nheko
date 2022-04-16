// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko

ApplicationWindow {
    id: rawMessageRoot

    property alias rawMessage: rawMessageView.text

    color: timelineRoot.palette.window
    flags: Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 420
    palette: timelineRoot.palette
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
        palette: timelineRoot.palette

        TextArea {
            id: rawMessageView
            anchors.fill: parent
            color: timelineRoot.palette.text
            font: Nheko.monospaceFont()
            readOnly: true
            textFormat: Text.PlainText

            background: Rectangle {
                color: timelineRoot.palette.base
            }
        }
    }
}
