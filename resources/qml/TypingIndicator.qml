// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Item {
    property var room: TimelineManager.timeline

    implicitHeight: Math.max(fontMetrics.height * 1.2, typingDisplay.height)
    Layout.fillWidth: true

    Rectangle {
        id: typingRect

        visible: (room && room.typingUsers.length > 0)
        color: colors.base
        anchors.fill: parent
        z: 3

        Label {
            id: typingDisplay

            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.bottom: parent.bottom
            color: colors.text
            text: room ? room.formatTypingUsers(room.typingUsers, colors.base) : ""
            textFormat: Text.RichText
        }

    }

}
