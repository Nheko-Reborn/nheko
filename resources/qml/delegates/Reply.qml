// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import im.nheko 1.0

Item {
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
    property string userId
    property string userName
    property string thumbnailUrl

    width: parent.width
    height: replyContainer.height

    TapHandler {
        onSingleTapped: chat.model.showEvent(eventId)
        gesturePolicy: TapHandler.ReleaseWithinBounds
    }

    CursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

    Rectangle {
        id: colorLine

        anchors.top: replyContainer.top
        anchors.bottom: replyContainer.bottom
        width: 4
        color: TimelineManager.userColor(userId, Nheko.colors.window)
    }

    Column {
        id: replyContainer

        anchors.left: colorLine.right
        anchors.leftMargin: 4
        width: parent.width - 8

        Text {
            id: userName_

            text: TimelineManager.escapeEmoji(userName)
            color: r.userColor
            textFormat: Text.RichText

            TapHandler {
                onSingleTapped: chat.model.openUserProfile(userId)
                gesturePolicy: TapHandler.ReleaseWithinBounds
            }

        }

        MessageDelegate {
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
            originalWidth: r.originalWidth
            isOnlyEmoji: r.isOnlyEmoji
            userId: r.userId
            userName: r.userName
            enabled: false
            width: parent.width
            isReply: true
        }

    }

    Rectangle {
        id: backgroundItem

        z: -1
        height: replyContainer.height
        width: Math.min(Math.max(reply.implicitWidth, userName_.implicitWidth) + 8 + 4, parent.width)
        color: Qt.rgba(userColor.r, userColor.g, userColor.b, 0.1)
    }

}
