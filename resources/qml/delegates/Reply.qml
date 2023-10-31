// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import im.nheko
import "../"

AbstractButton {
    id: r

    required property string eventId
    property bool keepFullText: false
    required property int maxWidth
    property var room_: room
    property color userColor: "red"
    property string userId: eventId ? room.dataById(eventId, Room.UserId, "") : ""
    property string userName: eventId ? room.dataById(eventId, Room.UserName, "") : ""

    implicitHeight: replyContainer.implicitHeight
    implicitWidth: replyContainer.implicitWidth

    background: Rectangle {
        id: backgroundItem

        property color bgColor: palette.base
        property color userColor: TimelineManager.userColor(r.userId, palette.base)

        color: Qt.tint(bgColor, Qt.hsla(userColor.hslHue, 0.5, userColor.hslLightness, 0.1))
        z: -1
    }
    contentItem: TimelineEvent {
        id: timelineEvent

        eventId: r.eventId
        isStateEvent: false
        mainInset: 4 + Nheko.paddingMedium
        maxWidth: r.maxWidth
        replyTo: ""
        room: r.room_

        //height: replyContainer.implicitHeight
        data: Row {
            id: replyContainer

            spacing: Nheko.paddingSmall

            Rectangle {
                id: colorline

                color: TimelineManager.userColor(r.userId, palette.base)
                height: content.height
                width: 4
            }
            Column {
                id: content

                data: [usernameBtn, timelineEvent.main,]
                spacing: 0

                AbstractButton {
                    id: usernameBtn

                    contentItem: Label {
                        id: userName_

                        color: r.userColor
                        text: r.userName
                        textFormat: Text.RichText
                        width: timelineEvent.main?.width
                    }

                    onClicked: room.openUserProfile(r.userId)
                }
            }
        }
    }

    onClicked: {
        let link = reply.child.linkAt != undefined && reply.child.linkAt(pressX - colorline.width, pressY - userName_.implicitHeight);
        if (link) {
            Nheko.openLink(link);
        } else {
            room.showEvent(r.eventId);
        }
    }
    onPressAndHold: replyContextMenu.show(reply.child.copyText, reply.child.linkAt(pressX - colorline.width, pressY - userName_.implicitHeight), r.eventId)

    NhekoCursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }
}
