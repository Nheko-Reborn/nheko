// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
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
    height: row.height+reactionRow.height+(Settings.bubbles? 8 : 4)

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

    Control {
        id: row

        anchors.rightMargin: 1
        anchors.leftMargin: Nheko.avatarSize + 12 // align bubble with section header
        anchors.left: parent.left
        anchors.right: parent.right
        height: msg.height
        topInset: -4
        bottomInset: -4
        leftInset: -4
        rightInset: -4
        background: Rectangle {
            //anchors.fill: msg
            property color userColor: TimelineManager.userColor(userId, Nheko.colors.base)
            property color bgColor: Nheko.colors.base
            color: Qt.rgba(userColor.r*0.1+bgColor.r*0.9,userColor.g*0.1+bgColor.g*0.9,userColor.b*0.1+bgColor.b*0.9,1) //TimelineManager.userColor(userId, Nheko.colors.base)
            radius: 4
            visible: Settings.bubbles
        }

        GridLayout {
            id: msg
            anchors {
                right: parent.right
                left: parent.left
                top: parent.top
            }
            property bool narrowLayout: (row.width < 350) && Settings.bubbles
            rowSpacing: 0
            columnSpacing: 2
            columns: narrowLayout? 1 : 2
            rows: narrowLayout? 3 : 2

            // fancy reply, if this is a reply
            Reply {
                Layout.row: 0
                Layout.column: 0
                Layout.fillWidth: true
                Layout.bottomMargin: visible? 2 : 0
                id: reply

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
                Layout.row: 1
                Layout.column: 0
                Layout.fillWidth: true
                id: contentItem

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

            RowLayout {
                Layout.column: msg.narrowLayout? 0 : 1
                Layout.row: msg.narrowLayout? 2 : 0
                Layout.rowSpan: msg.narrowLayout? 1 : 2
                Layout.bottomMargin: msg.narrowLayout? -4 : 0
                Layout.alignment: Qt.AlignTop | Qt.AlignRight

                property double scaling: msg.narrowLayout? 0.75 : 1
                StatusIndicator {
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    Layout.preferredHeight: 16*parent.scaling
                    width: 16*parent.scaling
                    status: r.status
                    eventId: r.eventId
                }

                Image {
                    visible: isEdited || eventId == chat.model.edit
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    Layout.preferredHeight: 16*parent.scaling
                    Layout.preferredWidth: 16*parent.scaling
                    height: 16*parent.scaling
                    width: 16*parent.scaling
                    sourceSize.width: 16 * Screen.devicePixelRatio*parent.scaling
                    sourceSize.height: 16 * Screen.devicePixelRatio*parent.scaling
                    source: "image://colorimage/:/icons/icons/ui/edit.svg?" + ((eventId == chat.model.edit) ? Nheko.colors.highlight : Nheko.colors.buttonText)
                    ToolTip.visible: editHovered.hovered
                    ToolTip.delay: Nheko.tooltipDelay
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
                    Layout.preferredHeight: 16*parent.scaling
                    Layout.preferredWidth: 16*parent.scaling
                }

                Label {
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    text: timestamp.toLocaleTimeString(Locale.ShortFormat)
                    width: Math.max(implicitWidth, text.length * fontMetrics.maximumCharacterWidth)
                    color: Nheko.inactiveColors.text
                    ToolTip.visible: ma.hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: Qt.formatDateTime(timestamp, Qt.DefaultLocaleLongDate)
                    font.pointSize: 10*parent.scaling
                    HoverHandler {
                        id: ma
                    }

                }
            }
        }
    }
    Reactions {
        anchors {
            top: row.bottom
            left: parent.left
            leftMargin: Nheko.avatarSize + 16
        }

        id: reactionRow

        reactions: r.reactions
        eventId: r.eventId
    }
}
