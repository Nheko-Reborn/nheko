// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import Qt.labs.platform 1.1 as Platform
import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0
import "../"

AbstractButton {
    id: r

    property color userColor: "red"
    property double proportionalHeight
    property int type
    property string typeString
    property int originalWidth
    property string blurhash
    property string body
    property string formattedBody
    property string eventId
    property string filename
    property string filesize
    property string url
    property bool isOnlyEmoji
    property bool isStateEvent
    property string userId
    property string userName
    property string thumbnailUrl
    property string roomTopic
    property string roomName
    property string callType
    property int duration
    property int encryptionError
    property int relatedEventCacheBuster
    property int maxWidth
    property bool keepFullText: false

    height: replyContainer.height
    implicitHeight: replyContainer.height
    implicitWidth: visible? colorLine.width+Math.max(replyContainer.implicitWidth,userName_.fullTextWidth) : 0 // visible? seems to be causing issues

    CursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

    Rectangle {
        id: colorLine

        anchors.top: replyContainer.top
        anchors.bottom: replyContainer.bottom
        width: 4
        color: TimelineManager.userColor(userId, Nheko.colors.base)
    }

    onClicked: {
        let link = reply.child.linkAt != undefined && reply.child.linkAt(pressX-colorLine.width, pressY - userName_.implicitHeight);
        if (link) {
            Nheko.openLink(link)
        } else {
            room.showEvent(r.eventId)
        }
    }
    onPressAndHold: replyContextMenu.show(reply.child.copyText, reply.child.linkAt(pressX-colorLine.width, pressY - userName_.implicitHeight), r.eventId)

    ColumnLayout {
        id: replyContainer

        anchors.left: colorLine.right
        width: parent.width - 4
        spacing: 0

        TapHandler {
            acceptedButtons: Qt.RightButton
            onSingleTapped: replyContextMenu.show(reply.child.copyText, reply.child.linkAt(eventPoint.position.x, eventPoint.position.y - userName_.implicitHeight), r.eventId)
            gesturePolicy: TapHandler.ReleaseWithinBounds
            acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
        }

        AbstractButton {
            Layout.leftMargin: 4
            Layout.fillWidth: true
            contentItem: ElidedLabel {
                id: userName_
                fullText: userName
                color: r.userColor
                textFormat: Text.RichText
                width: parent.width
                elideWidth: width
            }
            onClicked: room.openUserProfile(userId)
        }

        MessageDelegate {
            Layout.leftMargin: 4
            Layout.preferredHeight: height
            id: reply
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
            relatedEventCacheBuster: r.relatedEventCacheBuster
            encryptionError: r.encryptionError
            // This is disabled so that left clicking the reply goes to its location
            enabled: false
            Layout.fillWidth: true
            isReply: true
            keepFullText: r.keepFullText
        }

    }

    Rectangle {
        id: backgroundItem

        z: -1
        anchors.fill: replyContainer
        property color userColor: TimelineManager.userColor(userId, Nheko.colors.base)
        property color bgColor: Nheko.colors.base
        color: Qt.tint(bgColor, Qt.hsla(userColor.hslHue, 0.5, userColor.hslLightness, 0.1))
    }

}
