// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko

Item {
    Layout.fillWidth: true
    height: implicitHeight
    implicitHeight: warningRect.visible ? warningDisplay.implicitHeight : 0

    Rectangle {
        id: warningRect
        anchors.fill: parent
        color: timelineRoot.palette.base
        visible: (room && room.permissions.canPingRoom() && room.input.containsAtRoom)
        z: 3

        Label {
            id: warningDisplay
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10
            color: Nheko.theme.red
            text: qsTr("You are about to notify the whole room")
            textFormat: Text.PlainText
        }
    }
}
