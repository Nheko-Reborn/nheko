// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko

Item {
    Layout.fillWidth: true
    implicitHeight: Math.max(fontMetrics.height * 1.2, typingDisplay.height)

    Rectangle {
        id: typingRect
        anchors.fill: parent
        color: timelineRoot.palette.base
        visible: (room && room.typingUsers.length > 0)
        z: 3

        Label {
            id: typingDisplay
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10
            color: timelineRoot.palette.text
            text: room ? room.formatTypingUsers(room.typingUsers, timelineRoot.palette.base) : ""
            textFormat: Text.RichText
        }
    }
}
