// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import Qt.labs.platform 1.1 as P
import QtQuick
import im.nheko

P.MessageDialog {
    id: leaveRoomRoot

    property string reason: ""
    required property string roomId

    buttons: P.MessageDialog.Ok | P.MessageDialog.Cancel
    modality: Qt.ApplicationModal
    text: qsTr("Are you sure you want to leave?")
    title: qsTr("Leave room")

    onAccepted: {
        if (CallManager.haveCallInvite) {
            callManager.rejectInvite();
        } else if (CallManager.isOnCall) {
            CallManager.hangUp();
        }
        Rooms.leave(roomId, reason);
    }
}
