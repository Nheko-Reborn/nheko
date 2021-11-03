// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import Qt.labs.platform 1.1
import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

MessageDialog {
    id: leaveRoomRoot

    required property string roomId

    title: qsTr("Leave room")
    text: qsTr("Are you sure you want to leave?")
    modality: Qt.ApplicationModal
    buttons: Dialog.Ok | Dialog.Cancel
    onAccepted: Rooms.leave(roomId)
}
