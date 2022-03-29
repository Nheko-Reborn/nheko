// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates"
import "./emoji"
import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0

AbstractButton {
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
    required property bool isStateEvent
    required property string replyTo
    required property string userId
    required property string userName
    required property string roomTopic
    required property string roomName
    required property string callType
    required property var reactions
    required property int trustlevel
    required property int encryptionError
    required property int duration
    required property var timestamp
    required property int status
    required property int relatedEventCacheBuster

    hoverEnabled: true

    width: parent.width
    height: row.height+(reactionRow.height > 0 ? reactionRow.height-2 : 0 )

    Rectangle {
        color: (Settings.messageHoverHighlight && hovered) ? Nheko.colors.alternateBase : "transparent"
        anchors.fill: parent
        // this looks better without margins
        TapHandler {
            acceptedButtons: Qt.RightButton
            onSingleTapped: messageContextMenu.show(eventId, type, isSender, isEncrypted, isEditable, contentItem.child.hoveredLink, contentItem.child.copyText)
            gesturePolicy: TapHandler.ReleaseWithinBounds
            acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
        }
    }


    onPressAndHold: messageContextMenu.show(eventId, type, isSender, isEncrypted, isEditable, contentItem.child.hoveredLink, contentItem.child.copyText)
    onDoubleClicked: chat.model.reply = eventId

    DragHandler {
        id: draghandler
        yAxis.enabled: false
        xAxis.maximum: 100
        xAxis.minimum: -100
        onActiveChanged: {
            if(!active && (x < -70 || x > 70))
                chat.model.reply = eventId
        }
    }
    states: State {
        name: "dragging"
        when: draghandler.active
    }
    transitions: Transition {
        from: "dragging"
        to: ""
        PropertyAnimation {
            target: r
            properties: "x"
            easing.type: Easing.InOutQuad
            to: 0
            duration: 100
        }
    }

    onClicked: {
        let link = contentItem.child.linkAt != undefined && contentItem.child.linkAt(pressX-row.x-msg.x, pressY-row.y-msg.y-contentItem.y);
        if (link) {
            Nheko.openLink(link)
        }
    }

    Rectangle {
        id: row
        property bool bubbleOnRight : isSender && Settings.bubbles
        anchors.leftMargin: isStateEvent || Settings.smallAvatars? 0 : Nheko.avatarSize+8 // align bubble with section header
        anchors.left: isStateEvent? undefined : (bubbleOnRight? undefined : parent.left)
        anchors.right: isStateEvent? undefined: (bubbleOnRight? parent.right : undefined)
        anchors.horizontalCenter: isStateEvent? parent.horizontalCenter : undefined
        property int maxWidth: (parent.width-(Settings.smallAvatars || isStateEvent? 0 : Nheko.avatarSize+8))*(Settings.bubbles && !isStateEvent? 0.9 : 1)
        width: Settings.bubbles? Math.min(maxWidth,Math.max(reply.implicitWidth+8,contentItem.implicitWidth+metadata.width+20)) : maxWidth
        height: msg.height+msg.anchors.margins*2

        property color userColor: TimelineManager.userColor(userId, Nheko.colors.base)
        property color bgColor: Nheko.colors.base
        color: (Settings.bubbles && !isStateEvent) ? Qt.tint(bgColor, Qt.hsla(userColor.hslHue, 0.5, userColor.hslLightness, 0.2)) : "#00000000"
        radius: 4

        GridLayout {
            anchors {
                left: parent.left
                top: parent.top
                right: parent.right
                margins: (Settings.bubbles && ! isStateEvent)? 4 : 2
                leftMargin: 4
            }
            id: msg
            rowSpacing: 0
            columnSpacing: 2
            columns: Settings.bubbles? 1 : 2
            rows: Settings.bubbles? 3 : 2

            // fancy reply, if this is a reply
            Reply {
                Layout.row: 0
                Layout.column: 0
                Layout.fillWidth: true
                Layout.maximumWidth: Settings.bubbles? Number.MAX_VALUE : implicitWidth
                Layout.bottomMargin: visible? 2 : 0
                Layout.preferredHeight: height
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
                isStateEvent: r.relatedEventCacheBuster, fromModel(Room.IsStateEvent) ?? false
                userId: r.relatedEventCacheBuster, fromModel(Room.UserId) ?? ""
                userName: r.relatedEventCacheBuster, fromModel(Room.UserName) ?? ""
                thumbnailUrl: r.relatedEventCacheBuster, fromModel(Room.ThumbnailUrl) ?? ""
                duration: r.relatedEventCacheBuster, fromModel(Room.Duration) ?? ""
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
                Layout.preferredHeight: height
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
                duration: r.duration
                originalWidth: r.originalWidth
                isOnlyEmoji: r.isOnlyEmoji
                isStateEvent: r.isStateEvent
                userId: r.userId
                userName: r.userName
                roomTopic: r.roomTopic
                roomName: r.roomName
                callType: r.callType
                encryptionError: r.encryptionError
                relatedEventCacheBuster: r.relatedEventCacheBuster
                isReply: false
                metadataWidth: metadata.width
            }

            Row {
                id: metadata
                Layout.column: Settings.bubbles? 0 : 1
                Layout.row: Settings.bubbles? 2 : 0
                Layout.rowSpan: Settings.bubbles? 1 : 2
                Layout.bottomMargin: -2
                Layout.topMargin: (contentItem.fitsMetadata && Settings.bubbles)? -height-Layout.bottomMargin : 0
                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                Layout.preferredWidth: implicitWidth
                visible: !isStateEvent
                spacing: 2

                property double scaling: Settings.bubbles? 0.75 : 1

                property int iconSize: Math.floor(fontMetrics.ascent*scaling)

                StatusIndicator {
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    height: parent.iconSize
                    width: parent.iconSize
                    status: r.status
                    eventId: r.eventId
                    anchors.verticalCenter: ts.verticalCenter
                }

                Image {
                    visible: isEdited || eventId == chat.model.edit
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    height: parent.iconSize
                    width: parent.iconSize
                    sourceSize.width: parent.iconSize * Screen.devicePixelRatio
                    sourceSize.height: parent.iconSize * Screen.devicePixelRatio
                    source: "image://colorimage/:/icons/icons/ui/edit.svg?" + ((eventId == chat.model.edit) ? Nheko.colors.highlight : Nheko.colors.buttonText)
                    ToolTip.visible: editHovered.hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Edited")
                    anchors.verticalCenter: ts.verticalCenter

                    HoverHandler {
                        id: editHovered
                    }

                }

                EncryptionIndicator {
                    visible: room.isEncrypted
                    encrypted: isEncrypted
                    trust: trustlevel
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    height: parent.iconSize
                    width: parent.iconSize
                    sourceSize.width: parent.iconSize * Screen.devicePixelRatio
                    sourceSize.height: parent.iconSize * Screen.devicePixelRatio
                    anchors.verticalCenter: ts.verticalCenter
                }

                Label {
                    id: ts
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    Layout.preferredWidth: implicitWidth
                    text: timestamp.toLocaleTimeString(Locale.ShortFormat)
                    color: Nheko.inactiveColors.text
                    ToolTip.visible: ma.hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: Qt.formatDateTime(timestamp, Qt.DefaultLocaleLongDate)
                    font.pointSize: fontMetrics.font.pointSize*parent.scaling
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
            topMargin: -2
            left: row.bubbleOnRight? undefined : row.left
            right: row.bubbleOnRight? row.right : undefined
        }
        width: row.maxWidth
        layoutDirection: row.bubbleOnRight? Qt.RightToLeft : Qt.LeftToRight

        id: reactionRow

        reactions: r.reactions
        eventId: r.eventId
    }
}
