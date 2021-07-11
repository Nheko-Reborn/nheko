// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates"
import "./emoji"
import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import im.nheko 1.0

Item {
    id: r

    required property double proportionalHeight
    required property int type
    required property string typeString
    required property int originalWidth
    required property string blurhash
    required property string body
    required property string formattedBody
    required property string eventId
    required property string filename
    required property string filesize
    required property string url
    required property string thumbnailUrl
    required property bool isOnlyEmoji
    required property bool isSender
    required property bool isEncrypted
    required property bool isEditable
    required property bool isEdited
    required property string replyTo
    required property string userId
    required property string userName
    required property var reactions
    required property int trustlevel
    required property var timestamp
    required property int status

    anchors.left: parent.left
    anchors.right: parent.right
    height: row.height

    Rectangle {
        color: (Settings.messageHoverHighlight && hoverHandler.hovered) ? Nheko.colors.alternateBase : "transparent"
        anchors.fill: row
    }

    HoverHandler {
        id: hoverHandler

        acceptedDevices: PointerDevice.GenericPointer
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onSingleTapped: messageContextMenu.show(eventId, type, isSender, isEncrypted, isEditable, contentItem.child.hoveredLink, contentItem.child.copyText)
        gesturePolicy: TapHandler.ReleaseWithinBounds
    }

    TapHandler {
        onLongPressed: messageContextMenu.show(eventId, type, isSender, isEncrypted, isEditable, contentItem.child.hoveredLink, contentItem.child.copyText)
        onDoubleTapped: chat.model.reply = eventId
        gesturePolicy: TapHandler.ReleaseWithinBounds
    }

    RowLayout {
        id: row

        property var replyData: chat.model.getDump(replyTo, eventId)

        anchors.rightMargin: 1
        anchors.leftMargin: Nheko.avatarSize + 16
        anchors.left: parent.left
        anchors.right: parent.right

        Column {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 4
            Layout.topMargin: 1
            Layout.bottomMargin: 1

            // fancy reply, if this is a reply
            Reply {
                visible: replyTo
                userColor: TimelineManager.userColor(userId, Nheko.colors.base)
                blurhash: row.replyData.blurhash ?? ""
                body: row.replyData.body ?? ""
                formattedBody: row.replyData.formattedBody ?? ""
                eventId: row.replyData.eventId ?? ""
                filename: row.replyData.filename ?? ""
                filesize: row.replyData.filesize ?? ""
                proportionalHeight: row.replyData.proportionalHeight ?? 1
                type: row.replyData.type ?? MtxEvent.UnknownMessage
                typeString: row.replyData.typeString ?? ""
                url: row.replyData.url ?? ""
                originalWidth: row.replyData.originalWidth ?? 0
                isOnlyEmoji: row.replyData.isOnlyEmoji ?? false
                userId: row.replyData.userId ?? ""
                userName: row.replyData.userName ?? ""
                thumbnailUrl: row.replyData.thumbnailUrl ?? ""
            }

            // actual message content
            MessageDelegate {
                id: contentItem

                width: parent.width
                blurhash: r.blurhash
                body: r.body
                formattedBody: r.formattedBody
                eventId: r.eventId
                filename: r.filename
                filesize: r.filesize
                proportionalHeight: r.proportionalHeight
                type: r.type
                typeString: r.typeString ?? ""
                url: r.url
                thumbnailUrl: r.thumbnailUrl
                originalWidth: r.originalWidth
                isOnlyEmoji: r.isOnlyEmoji
                userId: r.userId
                userName: r.userName
                isReply: false
            }

            Reactions {
                id: reactionRow

                reactions: r.reactions
                eventId: r.eventId
            }

        }

        StatusIndicator {
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.preferredHeight: 16
            width: 16
            status: r.status
            eventId: r.eventId
        }

        EncryptionIndicator {
            visible: room.isEncrypted
            encrypted: isEncrypted
            trust: trustlevel
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.preferredHeight: 16
            Layout.preferredWidth: 16
        }

        Image {
            visible: isEdited || eventId == chat.model.edit
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.preferredHeight: 16
            Layout.preferredWidth: 16
            height: 16
            width: 16
            sourceSize.width: 16
            sourceSize.height: 16
            source: "image://colorimage/:/icons/icons/ui/edit.png?" + ((eventId == chat.model.edit) ? Nheko.colors.highlight : Nheko.colors.buttonText)
            ToolTip.visible: editHovered.hovered
            ToolTip.text: qsTr("Edited")

            HoverHandler {
                id: editHovered
            }

        }

        Label {
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            text: timestamp.toLocaleTimeString(Locale.ShortFormat)
            width: Math.max(implicitWidth, text.length * fontMetrics.maximumCharacterWidth)
            color: Nheko.inactiveColors.text
            ToolTip.visible: ma.hovered
            ToolTip.text: Qt.formatDateTime(timestamp, Qt.DefaultLocaleLongDate)

            HoverHandler {
                id: ma
            }

        }

    }

}
