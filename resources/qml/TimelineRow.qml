// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates"
import "./emoji"
import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
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
    required property string roomTopic
    required property string roomName
    required property string callType
    required property var reactions
    required property int trustlevel
    required property int encryptionError
    required property var timestamp
    required property int status
    required property int relatedEventCacheBuster

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
                function fromModel(role) {
                    return replyTo != "" ? room.dataById(replyTo, role, r.eventId) : null;
                }

                visible: replyTo
                userColor: r.relatedEventCacheBuster, TimelineManager.userColor(userId, Nheko.colors.base)
                blurhash: r.relatedEventCacheBuster, fromModel(Room.Blurhash) ?? ""
                body: r.relatedEventCacheBuster, fromModel(Room.Body) ?? ""
                formattedBody: r.relatedEventCacheBuster, fromModel(Room.FormattedBody) ?? ""
                eventId: fromModel(Room.EventId) ?? ""
                filename: r.relatedEventCacheBuster, fromModel(Room.Filename) ?? ""
                filesize: r.relatedEventCacheBuster, fromModel(Room.Filesize) ?? ""
                proportionalHeight: r.relatedEventCacheBuster, fromModel(Room.ProportionalHeight) ?? 1
                type: r.relatedEventCacheBuster, fromModel(Room.Type) ?? MtxEvent.UnknownMessage
                typeString: r.relatedEventCacheBuster, fromModel(Room.TypeString) ?? ""
                url: r.relatedEventCacheBuster, fromModel(Room.Url) ?? ""
                originalWidth: r.relatedEventCacheBuster, fromModel(Room.OriginalWidth) ?? 0
                isOnlyEmoji: r.relatedEventCacheBuster, fromModel(Room.IsOnlyEmoji) ?? false
                userId: r.relatedEventCacheBuster, fromModel(Room.UserId) ?? ""
                userName: r.relatedEventCacheBuster, fromModel(Room.UserName) ?? ""
                thumbnailUrl: r.relatedEventCacheBuster, fromModel(Room.ThumbnailUrl) ?? ""
                roomTopic: r.relatedEventCacheBuster, fromModel(Room.RoomTopic) ?? ""
                roomName: r.relatedEventCacheBuster, fromModel(Room.RoomName) ?? ""
                callType: r.relatedEventCacheBuster, fromModel(Room.CallType) ?? ""
                encryptionError: r.relatedEventCacheBuster, fromModel(Room.EncryptionError) ?? ""
                relatedEventCacheBuster: r.relatedEventCacheBuster, fromModel(Room.RelatedEventCacheBuster) ?? 0
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
                roomTopic: r.roomTopic
                roomName: r.roomName
                callType: r.callType
                encryptionError: r.encryptionError
                relatedEventCacheBuster: r.relatedEventCacheBuster
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

        EncryptionIndicator {
            visible: room.isEncrypted
            encrypted: isEncrypted
            trust: trustlevel
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.preferredHeight: 16
            Layout.preferredWidth: 16
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
