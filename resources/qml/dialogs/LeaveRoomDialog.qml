// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import Qt.labs.platform 1.1 as P
import QtQuick
import im.nheko

P.MessageDialog {
    id: leaveRoomRoot

    required property string roomId
    property string reason: ""

    title: qsTr("Leave room")
    text: qsTr("Are you sure you want to leave?")
    modality: Qt.ApplicationModal
    buttons: P.MessageDialog.Ok | P.MessageDialog.Cancel
    onAccepted: {

        if (CallManager.haveCallInvite) {
            callManager.rejectInvite();
        } else if (CallManager.isOnCall) {
            CallManager.hangUp();
        }
        Rooms.leave(roomId, reason)
    }
        
}
