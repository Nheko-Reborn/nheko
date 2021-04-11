// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import im.nheko 1.0

Item {
    id: replyComponent

    property alias modelData: reply.modelData
    property color userColor: "red"

    width: parent.width
    height: replyContainer.height

    TapHandler {
        onSingleTapped: chat.positionViewAtIndex(chat.model.idToIndex(modelData.id), ListView.Contain)
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
        color: TimelineManager.userColor(reply.modelData.userId, colors.window)
    }

    Column {
        id: replyContainer

        anchors.left: colorLine.right
        anchors.leftMargin: 4
        width: parent.width - 8

        Text {
            id: userName

            text: TimelineManager.escapeEmoji(reply.modelData.userName)
            color: replyComponent.userColor
            textFormat: Text.RichText

            TapHandler {
                onSingleTapped: chat.model.openUserProfile(reply.modelData.userId)
                gesturePolicy: TapHandler.ReleaseWithinBounds
            }

        }

        MessageDelegate {
            id: reply

            width: parent.width
            isReply: true
        }

    }

    Rectangle {
        id: backgroundItem

        z: -1
        height: replyContainer.height
        width: Math.min(Math.max(reply.implicitWidth, userName.implicitWidth) + 8 + 4, parent.width)
        color: Qt.rgba(userColor.r, userColor.g, userColor.b, 0.1)
    }

}
