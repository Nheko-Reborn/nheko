// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko 1.0

ApplicationWindow {
    id: joinRoomRoot

    title: qsTr("Join room")
    modality: Qt.WindowModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    palette: Nheko.colors
    color: Nheko.colors.window
    width: 350
    height: fontMetrics.lineSpacing * 7

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: dbb.rejected()
    }

    ColumnLayout {
        spacing: Nheko.paddingMedium
        anchors.margins: Nheko.paddingMedium
        anchors.fill: parent

        Label {
            id: promptLabel

            text: qsTr("Room ID or alias")
            color: Nheko.colors.text
        }

        MatrixTextField {
            id: input

            focus: true
            Layout.fillWidth: true
            onAccepted: {
                if (input.text.match("#.+?:.{3,}"))
                    dbb.accepted();

            }
        }

    }

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
            text: qsTr("Join")
            enabled: input.text.match("#.+?:.{3,}")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }

    }

}
