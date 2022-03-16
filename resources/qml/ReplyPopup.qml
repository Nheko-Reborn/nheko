// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates/"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Rectangle {
    id: replyPopup

    Layout.fillWidth: true
    visible: room && (room.reply || room.edit)
    // Height of child, plus margins, plus border
    implicitHeight: (room && room.reply ? replyPreview.height : closeEditButton.height) + Nheko.paddingSmall
    color: Nheko.colors.window
    z: 3

    Reply {
        id: replyPreview

        property var modelData: room ? room.getDump(room.reply, room.id) : {
        }

        visible: room && room.reply
        anchors.left: parent.left
        anchors.leftMargin: replyPopup.width < 450? Nheko.paddingSmall : (CallManager.callsSupported? 2*(22+16) : 1*(22+16))
        anchors.right: parent.right
        anchors.rightMargin: replyPopup.width < 450? 2*(22+16) : 3*(22+16)
        anchors.top: parent.top
        anchors.topMargin: Nheko.paddingSmall
        userColor: TimelineManager.userColor(modelData.userId, Nheko.colors.window)
        blurhash: modelData.blurhash ?? ""
        body: modelData.body ?? ""
        formattedBody: modelData.formattedBody ?? ""
        eventId: modelData.eventId ?? ""
        filename: modelData.filename ?? ""
        filesize: modelData.filesize ?? ""
        proportionalHeight: modelData.proportionalHeight ?? 1
        type: modelData.type ?? MtxEvent.UnknownMessage
        typeString: modelData.typeString ?? ""
        url: modelData.url ?? ""
        originalWidth: modelData.originalWidth ?? 0
        isOnlyEmoji: modelData.isOnlyEmoji ?? false
        userId: modelData.userId ?? ""
        userName: modelData.userName ?? ""
        encryptionError: modelData.encryptionError ?? ""
        width: parent.width
    }

    ImageButton {
        id: closeReplyButton

        visible: room && room.reply
        anchors.right: replyPreview.right
        anchors.top: replyPreview.top
        anchors.margins: Nheko.paddingSmall
        hoverEnabled: true
        width: 16
        height: 16
        image: ":/icons/icons/ui/dismiss.svg"
        ToolTip.visible: closeReplyButton.hovered
        ToolTip.text: qsTr("Close")
        onClicked: room.reply = undefined
    }

    ImageButton {
        id: closeEditButton

        visible: room && room.edit
        anchors.right: parent.right
        anchors.margins: 8
        anchors.top: parent.top
        hoverEnabled: true
        image: ":/icons/icons/ui/dismiss_edit.svg"
        width: 22
        height: 22
        ToolTip.visible: closeEditButton.hovered
        ToolTip.text: qsTr("Cancel Edit")
        onClicked: room.edit = undefined
    }

}
