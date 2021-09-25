// SPDX-FileCopyrightText: 2021 Nheko Contributors
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
    Component.onCompleted: Nheko.reparent(joinRoomRoot)
    width: 350
    height: fontMetrics.lineSpacing * 7

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

            Layout.fillWidth: true
        }

    }

    footer: DialogButtonBox {
        onAccepted: {
            Nheko.joinRoom(input.text);
            joinRoomRoot.close();
        }
        onRejected: {
            joinRoomRoot.close();
        }

        Button {
            text: "Join"
            enabled: input.text.match("#.+?:.{3,}")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }

        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }

}
