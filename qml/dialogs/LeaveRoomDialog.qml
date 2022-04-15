// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import Qt.labs.platform 1.1 as P
import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko

P.MessageDialog {
    id: leaveRoomRoot

    required property string roomId
    property string reason: ""

    title: qsTr("Leave room")
    text: qsTr("Are you sure you want to leave?")
    modality: Qt.ApplicationModal
    buttons: P.MessageDialog.Ok | P.MessageDialog.Cancel
    onAccepted: Rooms.leave(roomId, reason)
}
