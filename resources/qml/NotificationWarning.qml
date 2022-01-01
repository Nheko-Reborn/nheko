// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Item {
    implicitHeight: warningRect.visible ? warningDisplay.implicitHeight : 0
    height: implicitHeight
    Layout.fillWidth: true

    Rectangle {
        id: warningRect

        visible: (room && room.permissions.canPingRoom() && room.input.containsAtRoom)
        color: Nheko.colors.base
        anchors.fill: parent
        z: 3

        Label {
            id: warningDisplay

            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.bottom: parent.bottom
            color: Nheko.theme.red
            text: qsTr("You are about to notify the whole room")
            textFormat: Text.PlainText
        }

    }

}
