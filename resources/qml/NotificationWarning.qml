// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Item {
    implicitHeight: warningRect.visible ? warningDisplay.implicitHeight + 2 * Nheko.paddingSmall : 0
    height: implicitHeight
    Layout.fillWidth: true

    Rectangle {
        id: warningRect

        visible: (room && room.permissions.canPingRoom() && room.input.containsAtRoom)
        // TODO: Qt.alpha() would make more sense but it wasn't working...
        color: Qt.rgba(Nheko.theme.error.r, Nheko.theme.error.g, Nheko.theme.error.b, 0.3)
        border.width: 1
        border.color: Nheko.theme.error
        radius: 3
        anchors.fill: parent
        z: 3

        Label {
            id: warningDisplay

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: Nheko.paddingSmall
            color: Nheko.colors.text
            text: qsTr("You are about to notify the whole room")
            textFormat: Text.PlainText
        }

    }

}
