// SPDX-FileCopyrightText: Nheko Contributors
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

    required property string blurhash
    required property string body
    required property string callType
    required property int duration
    required property int encryptionError
    required property string eventId
    required property string filename
    required property string filesize
    required property string formattedBody
    required property int index
    required property bool isEditable
    required property bool isEdited
    required property bool isEncrypted
    required property bool isOnlyEmoji
    required property bool isSender
    required property bool isStateEvent
    required property int notificationlevel
    required property int originalWidth
    required property double proportionalHeight
    required property var reactions
    required property int relatedEventCacheBuster
    required property string replyTo
    required property string roomName
    required property string roomTopic
    required property int status
    required property string threadId
    required property string thumbnailUrl
    required property var timestamp
    required property int trustlevel
    required property int type
    required property string typeString
    required property string url
    required property string userId
    required property string userName

    height: row.height + (reactionRow.height > 0 ? reactionRow.height - 2 : 0) + unreadRow.height
    hoverEnabled: true

    states: State {
        name: "dragging"
        when: draghandler.active
    }
    transitions: Transition {
        from: "dragging"
        to: ""

        PropertyAnimation {
            duration: 100
            easing.type: Easing.InOutQuad
            properties: "x"
            target: r
            to: 0
        }
    }

    onClicked: {
        let link = contentItem.child.linkAt != undefined && contentItem.child.linkAt(pressX - row.x - msg.x, pressY - row.y - msg.y - contentItem.y);
        if (link) {
            Nheko.openLink(link);
        }
    }
    onDoubleClicked: room.reply = eventId
    onPressAndHold: messageContextMenu.show(eventId, threadId, type, isSender, isEncrypted, isEditable, contentItem.child.hoveredLink, contentItem.child.copyText)

    Rectangle {
        anchors.fill: parent
        color: (Settings.messageHoverHighlight && hovered) ? palette.alternateBase : "transparent"

        // this looks better without margins
        TapHandler {
            acceptedButtons: Qt.RightButton
            acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
            gesturePolicy: TapHandler.ReleaseWithinBounds

            onSingleTapped: messageContextMenu.show(eventId, threadId, type, isSender, isEncrypted, isEditable, contentItem.child.hoveredLink, contentItem.child.copyText)
        }
    }
    DragHandler {
        id: draghandler

        xAxis.maximum: 100
        xAxis.minimum: -100
        yAxis.enabled: false

        onActiveChanged: {
            if (!active && (x < -70 || x > 70))
                room.reply = eventId;
        }
    }
    AbstractButton {
        ToolTip.delay: Nheko.tooltipDelay
        ToolTip.text: qsTr("Part of a thread")
        ToolTip.visible: hovered
        anchors.left: parent.left
        anchors.leftMargin: Settings.smallAvatars ? 0 : (Nheko.avatarSize + 8) // align bubble with section header
        height: parent.height
        visible: threadId
        width: 4

        onClicked: room.thread = threadId

        Rectangle {
            id: threadLine

            anchors.fill: parent
            color: TimelineManager.userColor(threadId, palette.base)
        }
    }
    Rectangle {
        id: row

        property color bgColor: palette.base
        property bool bubbleOnRight: isSender && Settings.bubbles
        property int maxWidth: (parent.width - (Settings.smallAvatars || isStateEvent ? 0 : Nheko.avatarSize + 8)) * (Settings.bubbles && !isStateEvent ? 0.9 : 1)
        property color userColor: TimelineManager.userColor(userId, palette.base)

        anchors.horizontalCenter: isStateEvent ? parent.horizontalCenter : undefined
        anchors.left: (isStateEvent || bubbleOnRight) ? undefined : parent.left
        anchors.leftMargin: (isStateEvent || Settings.smallAvatars ? 0 : (Nheko.avatarSize + 8)) + (threadId ? 6 : 0) // align bubble with section header
        anchors.right: (isStateEvent || !bubbleOnRight) ? undefined : parent.right
        border.color: Nheko.theme.red
        border.width: r.notificationlevel == MtxEvent.Highlight ? 1 : 0
        color: (Settings.bubbles && !isStateEvent) ? Qt.tint(bgColor, Qt.hsla(userColor.hslHue, 0.5, userColor.hslLightness, 0.2)) : "#00000000"
        height: msg.height + msg.anchors.margins * 2
        radius: 4
        width: Settings.bubbles ? Math.min(maxWidth, Math.max(reply.implicitWidth + 8, contentItem.implicitWidth + metadata.width + 20)) : maxWidth

        GridLayout {
            id: msg

            columnSpacing: 2
            columns: Settings.bubbles ? 1 : 2
            rowSpacing: 0
            rows: Settings.bubbles ? 3 : 2

            anchors {
                left: parent.left
                leftMargin: 4
                margins: (Settings.bubbles && !isStateEvent) ? 4 : 2
                right: parent.right
                rightMargin: 4
                top: parent.top
            }

            // fancy reply, if this is a reply
            Reply {
                id: reply

                function fromModel(role) {
                    return replyTo != "" ? room.dataById(replyTo, role, r.eventId) : null;
                }

                Layout.bottomMargin: visible ? 2 : 0
                Layout.column: 0
                Layout.fillWidth: true
                Layout.maximumWidth: Settings.bubbles ? Number.MAX_VALUE : implicitWidth
                Layout.preferredHeight: height
                Layout.row: 0
                blurhash: r.relatedEventCacheBuster, fromModel(Room.Blurhash) ?? ""
                body: r.relatedEventCacheBuster, fromModel(Room.Body) ?? ""
                callType: r.relatedEventCacheBuster, fromModel(Room.Voip) ?? ""
                duration: r.relatedEventCacheBuster, fromModel(Room.Duration) ?? 0
                encryptionError: r.relatedEventCacheBuster, fromModel(Room.EncryptionError) ?? 0
                eventId: fromModel(Room.EventId) ?? ""
                filename: r.relatedEventCacheBuster, fromModel(Room.Filename) ?? ""
                filesize: r.relatedEventCacheBuster, fromModel(Room.Filesize) ?? ""
                formattedBody: r.relatedEventCacheBuster, fromModel(Room.FormattedBody) ?? ""
                isOnlyEmoji: r.relatedEventCacheBuster, fromModel(Room.IsOnlyEmoji) ?? false
                isStateEvent: r.relatedEventCacheBuster, fromModel(Room.IsStateEvent) ?? false
                originalWidth: r.relatedEventCacheBuster, fromModel(Room.OriginalWidth) ?? 0
                proportionalHeight: r.relatedEventCacheBuster, fromModel(Room.ProportionalHeight) ?? 1
                relatedEventCacheBuster: r.relatedEventCacheBuster, fromModel(Room.RelatedEventCacheBuster) ?? 0
                roomName: r.relatedEventCacheBuster, fromModel(Room.RoomName) ?? ""
                roomTopic: r.relatedEventCacheBuster, fromModel(Room.RoomTopic) ?? ""
                thumbnailUrl: r.relatedEventCacheBuster, fromModel(Room.ThumbnailUrl) ?? ""
                type: r.relatedEventCacheBuster, fromModel(Room.Type) ?? MtxEvent.UnknownMessage
                typeString: r.relatedEventCacheBuster, fromModel(Room.TypeString) ?? ""
                url: r.relatedEventCacheBuster, fromModel(Room.Url) ?? ""
                userColor: r.relatedEventCacheBuster, TimelineManager.userColor(userId, palette.base)
                userId: r.relatedEventCacheBuster, fromModel(Room.UserId) ?? ""
                userName: r.relatedEventCacheBuster, fromModel(Room.UserName) ?? ""
                visible: replyTo
            }

            // actual message content
            MessageDelegate {
                id: contentItem

                Layout.column: 0
                Layout.fillWidth: true
                Layout.preferredHeight: height
                Layout.row: 1
                blurhash: r.blurhash
                body: r.body
                callType: r.callType
                duration: r.duration
                encryptionError: r.encryptionError
                eventId: r.eventId
                filename: r.filename
                filesize: r.filesize
                formattedBody: r.formattedBody
                isOnlyEmoji: r.isOnlyEmoji
                isReply: false
                isStateEvent: r.isStateEvent
                metadataWidth: metadata.width
                originalWidth: r.originalWidth
                proportionalHeight: r.proportionalHeight
                relatedEventCacheBuster: r.relatedEventCacheBuster
                roomName: r.roomName
                roomTopic: r.roomTopic
                thumbnailUrl: r.thumbnailUrl
                type: r.type
                typeString: r.typeString ?? ""
                url: r.url
                userId: r.userId
                userName: r.userName
            }
            Row {
                id: metadata

                property int iconSize: Math.floor(fontMetrics.ascent * scaling)
                property double scaling: Settings.bubbles ? 0.75 : 1

                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                Layout.bottomMargin: -2
                Layout.column: Settings.bubbles ? 0 : 1
                Layout.preferredWidth: implicitWidth
                Layout.row: Settings.bubbles ? 2 : 0
                Layout.rowSpan: Settings.bubbles ? 1 : 2
                Layout.topMargin: (contentItem.fitsMetadata && Settings.bubbles) ? -height - Layout.bottomMargin : 0
                spacing: 2
                visible: !isStateEvent

                StatusIndicator {
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    anchors.verticalCenter: ts.verticalCenter
                    eventId: r.eventId
                    height: parent.iconSize
                    status: r.status
                    width: parent.iconSize
                }
                Image {
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Edited")
                    ToolTip.visible: editHovered.hovered
                    anchors.verticalCenter: ts.verticalCenter
                    height: parent.iconSize
                    source: "image://colorimage/:/icons/icons/ui/edit.svg?" + ((eventId == room.edit) ? palette.highlight : palette.buttonText)
                    sourceSize.height: parent.iconSize * Screen.devicePixelRatio
                    sourceSize.width: parent.iconSize * Screen.devicePixelRatio
                    visible: isEdited || eventId == room.edit
                    width: parent.iconSize

                    HoverHandler {
                        id: editHovered

                    }
                }
                ImageButton {
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Part of a thread")
                    ToolTip.visible: hovered
                    anchors.verticalCenter: ts.verticalCenter
                    buttonTextColor: TimelineManager.userColor(threadId, palette.base)
                    height: parent.iconSize
                    image: ":/icons/icons/ui/thread.svg"
                    visible: threadId
                    width: parent.iconSize

                    onClicked: room.thread = threadId
                }
                EncryptionIndicator {
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    anchors.verticalCenter: ts.verticalCenter
                    encrypted: isEncrypted
                    height: parent.iconSize
                    sourceSize.height: parent.iconSize * Screen.devicePixelRatio
                    sourceSize.width: parent.iconSize * Screen.devicePixelRatio
                    trust: trustlevel
                    visible: room.isEncrypted
                    width: parent.iconSize
                }
                Label {
                    id: ts

                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    Layout.preferredWidth: implicitWidth
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: Qt.formatDateTime(timestamp, Qt.DefaultLocaleLongDate)
                    ToolTip.visible: ma.hovered
                    color: palette.inactive.text
                    font.pointSize: fontMetrics.font.pointSize * parent.scaling
                    text: timestamp.toLocaleTimeString(Locale.ShortFormat)

                    HoverHandler {
                        id: ma

                    }
                }
            }
        }
    }
    Reactions {
        id: reactionRow

        eventId: r.eventId
        layoutDirection: row.bubbleOnRight ? Qt.RightToLeft : Qt.LeftToRight
        reactions: r.reactions
        width: row.maxWidth

        anchors {
            left: row.bubbleOnRight ? undefined : row.left
            right: row.bubbleOnRight ? row.right : undefined
            top: row.bottom
            topMargin: -4
        }
    }
    Rectangle {
        id: unreadRow

        color: palette.highlight
        height: visible ? 3 : 0
        visible: (r.index > 0 && (room.fullyReadEventId == r.eventId))

        anchors {
            left: parent.left
            right: parent.right
            top: reactionRow.bottom
            topMargin: 5
        }
    }
}
