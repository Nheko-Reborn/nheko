// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import Qt.labs.platform 1.1 as P
import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko

P.MessageDialog {
    id: leaveRoomRoot

    property string reason: ""
    required property string roomId

    buttons: P.MessageDialog.Ok | P.MessageDialog.Cancel
    modality: Qt.ApplicationModal
    text: qsTr("Are you sure you want to leave?")
    title: qsTr("Leave room")

    onAccepted: Rooms.leave(roomId, reason)
}
