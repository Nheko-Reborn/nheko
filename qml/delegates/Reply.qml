// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko
import "../"

AbstractButton {
    id: r

    property string blurhash
    property string body
    property string callType
    property int duration
    property int encryptionError
    property string eventId
    property string filename
    property string filesize
    property string formattedBody
    property bool isOnlyEmoji
    property bool isStateEvent
    property int maxWidth
    property int originalWidth
    property double proportionalHeight
    property int relatedEventCacheBuster
    property string roomName
    property string roomTopic
    property string thumbnailUrl
    property int type
    property string typeString
    property string url
    property color userColor: "red"
    property string userId
    property string userName

    height: replyContainer.height
    implicitHeight: replyContainer.height
    implicitWidth: visible ? colorLine.width + Math.max(replyContainer.implicitWidth, userName_.fullTextWidth) : 0 // visible? seems to be causing issues

    onClicked: {
        let link = reply.child.linkAt != undefined && reply.child.linkAt(pressX - colorLine.width, pressY - userName_.implicitHeight);
        if (link) {
            Nheko.openLink(link);
        } else {
            room.showEvent(r.eventId);
        }
    }
    onPressAndHold: replyContextMenu.show(reply.child.copyText, reply.child.linkAt(pressX - colorLine.width, pressY - userName_.implicitHeight), r.eventId)

    NhekoCursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }
    Rectangle {
        id: colorLine
        anchors.bottom: replyContainer.bottom
        anchors.top: replyContainer.top
        color: TimelineManager.userColor(userId, timelineRoot.palette.base)
        width: 4
    }
    ColumnLayout {
        id: replyContainer
        anchors.left: colorLine.right
        spacing: 0
        width: parent.width - 4

        TapHandler {
            acceptedButtons: Qt.RightButton
            acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
            gesturePolicy: TapHandler.ReleaseWithinBounds

            onSingleTapped: replyContextMenu.show(reply.child.copyText, reply.child.linkAt(eventPoint.position.x, eventPoint.position.y - userName_.implicitHeight), r.eventId)
        }
        AbstractButton {
            Layout.fillWidth: true
            Layout.leftMargin: 4

            contentItem: ElidedLabel {
                id: userName_
                color: r.userColor
                elideWidth: width
                fullText: userName
                textFormat: Text.RichText
                width: parent.width
            }

            onClicked: room.openUserProfile(userId)
        }
        MessageDelegate {
            id: reply
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.preferredHeight: height
            blurhash: r.blurhash
            body: r.body
            callType: r.callType
            duration: r.duration
            // This is disabled so that left clicking the reply goes to its location
            enabled: false
            encryptionError: r.encryptionError
            eventId: r.eventId
            filename: r.filename
            filesize: r.filesize
            formattedBody: r.formattedBody
            isOnlyEmoji: r.isOnlyEmoji
            isReply: true
            isStateEvent: r.isStateEvent
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
    }
    Rectangle {
        id: backgroundItem

        property color bgColor: timelineRoot.palette.base
        property color userColor: TimelineManager.userColor(userId, timelineRoot.palette.base)

        anchors.fill: replyContainer
        color: Qt.tint(bgColor, Qt.hsla(userColor.hslHue, 0.5, userColor.hslLightness, 0.1))
        z: -1
    }
}
