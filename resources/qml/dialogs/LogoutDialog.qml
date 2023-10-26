// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import Qt.labs.platform 1.1 as P
import QtQuick
import im.nheko

P.MessageDialog {
    id: logoutRoot

    title: qsTr("Log out")
    text: CallManager.isOnCall ? qsTr("A call is in progress. Log out?") : qsTr("Are you sure you want to log out?")
    modality: Qt.WindowModal
    flags: Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    buttons: P.MessageDialog.Ok | P.MessageDialog.Cancel
    onAccepted: Nheko.logout()
}
