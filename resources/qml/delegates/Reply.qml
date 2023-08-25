// SPDX-FileCopyrightText: Nheko Contributors
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
    property bool keepFullText: false

    required property string eventId

    property var room_: room

    property string userId: eventId ? room.dataById(eventId, Room.UserId, "") : ""
    property string userName: eventId ? room.dataById(eventId, Room.UserName, "") : ""
    implicitHeight: replyContainer.implicitHeight
    implicitWidth: replyContainer.implicitWidth

    NhekoCursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

    onClicked: {
        let link = reply.child.linkAt != undefined && reply.child.linkAt(pressX-colorline.width, pressY - userName_.implicitHeight);
        if (link) {
            Nheko.openLink(link)
        } else {
            room.showEvent(r.eventId)
        }
    }
    onPressAndHold: replyContextMenu.show(reply.child.copyText, reply.child.linkAt(pressX-colorline.width, pressY - userName_.implicitHeight), r.eventId)

    contentItem: TimelineEvent {
        id: timelineEvent

        isStateEvent: false
        room: room_
        eventId: r.eventId
        replyTo: ""

        width: parent.width
        height: replyContainer.implicitHeight

        //height: replyContainer.implicitHeight
        data: GridLayout {
            id: replyContainer

            width: parent.width
            columns: 2
            rows: 2
            columnSpacing: Nheko.paddingMedium
            rowSpacing: Nheko.paddingSmall

            Rectangle {
                id: colorline

                Layout.preferredWidth: 4
                Layout.rowSpan: 2
                Layout.fillHeight: true

                Layout.row: 0
                Layout.column: 0

                color: TimelineManager.userColor(r.userId, palette.base)
            }

            AbstractButton {
                id: usernameBtn
                Layout.fillWidth: true

                Layout.row: 0
                Layout.column: 1

                contentItem: ElidedLabel {
                    id: userName_
                    fullText: r.userName
                    color: r.userColor
                    textFormat: Text.RichText
                    width: parent.width
                    elideWidth: width
                }
                onClicked: room.openUserProfile(r.userId)
            }

            data: [
                colorline, usernameBtn, timelineEvent.main,
            ]

        }
    }

    background: Rectangle {
        id: backgroundItem

        z: -1
        property color userColor: TimelineManager.userColor(r.userId, palette.base)
        property color bgColor: palette.base
        color: Qt.tint(bgColor, Qt.hsla(userColor.hslHue, 0.5, userColor.hslLightness, 0.1))
    }

}
