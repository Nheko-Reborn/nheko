// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko 1.0

ApplicationWindow {
    id: joinRoomRoot

    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: fontMetrics.lineSpacing * 7
    modality: Qt.WindowModal
    title: qsTr("Join room")
    width: 350

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Cancel

        onAccepted: {
            Nheko.joinRoom(input.text);
            joinRoomRoot.close();
        }
        onRejected: {
            joinRoomRoot.close();
        }

        Button {
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            enabled: input.text.match("#.+?:.{3,}")
            text: qsTr("Join")
        }
    }

    Shortcut {
        sequence: StandardKey.Cancel

        onActivated: dbb.rejected()
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Label {
            id: promptLabel

            color: palette.text
            text: qsTr("Room ID or alias")
        }
        MatrixTextField {
            id: input

            Layout.fillWidth: true
            focus: true

            onAccepted: {
                if (input.text.match("#.+?:.{3,}"))
                    dbb.accepted();
            }
        }
    }
}
