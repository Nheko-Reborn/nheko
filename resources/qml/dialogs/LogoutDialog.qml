// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import Qt.labs.platform 1.1
import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

MessageDialog {
    id: logoutRoot

    title: qsTr("Log out")
    text: CallManager.isOnCall ? qsTr("A call is in progress. Log out?") : qsTr("Are you sure you want to log out?")
    modality: Qt.WindowModal
    flags: Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    buttons: Dialog.Ok | Dialog.Cancel
    onAccepted: Nheko.logout()
}
