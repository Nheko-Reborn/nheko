// SPDX-FileCopyrightText: Nheko Contributors
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
    color: palette.window
    // Height of child, plus margins, plus border
    implicitHeight: (room && room.reply ? replyPreview.height : Math.max(closeEditButton.height, closeThreadButton.height)) + Nheko.paddingSmall
    visible: room && (room.reply || room.edit || room.thread)
    z: 3

    Reply {
        id: replyPreview

        property var modelData: room ? room.getDump(room.reply, room.id) : {}

        anchors.left: parent.left
        anchors.leftMargin: replyPopup.width < 450 ? Nheko.paddingSmall : (CallManager.callsSupported ? 2 * (22 + 16) : 1 * (22 + 16))
        anchors.right: parent.right
        anchors.rightMargin: replyPopup.width < 450 ? 2 * (22 + 16) : 3 * (22 + 16)
        anchors.top: parent.top
        anchors.topMargin: Nheko.paddingSmall
        blurhash: modelData.blurhash ?? ""
        body: modelData.body ?? ""
        encryptionError: modelData.encryptionError ?? 0
        eventId: modelData.eventId ?? ""
        filename: modelData.filename ?? ""
        filesize: modelData.filesize ?? ""
        formattedBody: modelData.formattedBody ?? ""
        isOnlyEmoji: modelData.isOnlyEmoji ?? false
        originalWidth: modelData.originalWidth ?? 0
        proportionalHeight: modelData.proportionalHeight ?? 1
        type: modelData.type ?? MtxEvent.UnknownMessage
        typeString: modelData.typeString ?? ""
        url: modelData.url ?? ""
        userColor: TimelineManager.userColor(modelData.userId, palette.window)
        userId: modelData.userId ?? ""
        userName: modelData.userName ?? ""
        visible: room && room.reply
        width: parent.width
    }
    ImageButton {
        id: closeReplyButton

        ToolTip.text: qsTr("Close")
        ToolTip.visible: closeReplyButton.hovered
        anchors.margins: Nheko.paddingSmall
        anchors.right: replyPreview.right
        anchors.top: replyPreview.top
        height: 16
        hoverEnabled: true
        image: ":/icons/icons/ui/dismiss.svg"
        visible: room && room.reply
        width: 16

        onClicked: room.reply = undefined
    }
    ImageButton {
        id: closeEditButton

        ToolTip.text: qsTr("Cancel Edit")
        ToolTip.visible: closeEditButton.hovered
        anchors.margins: 8
        anchors.right: closeThreadButton.left
        anchors.top: parent.top
        height: 22
        hoverEnabled: true
        image: ":/icons/icons/ui/dismiss_edit.svg"
        visible: room && room.edit
        width: 22

        onClicked: room.edit = undefined
    }
    ImageButton {
        id: closeThreadButton

        ToolTip.text: qsTr("Cancel Thread")
        ToolTip.visible: closeThreadButton.hovered
        anchors.margins: 8
        anchors.right: parent.right
        anchors.top: parent.top
        buttonTextColor: room ? TimelineManager.userColor(room.thread, palette.base) : palette.buttonText
        height: 22
        hoverEnabled: true
        image: ":/icons/icons/ui/dismiss_thread.svg"
        visible: room && room.thread
        width: 22

        onClicked: room.thread = undefined
    }
}
