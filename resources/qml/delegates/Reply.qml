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

    property color userColor: "red"
    property bool keepFullText: false

    required property string eventId

    property var room_: room

    property string userId: eventId ? room.dataById(eventId, Room.UserId, "") : ""
    property string userName: eventId ? room.dataById(eventId, Room.UserName, "") : ""
    implicitHeight: replyContainer.height
    implicitWidth: replyContainer.implicitWidth
    required property int maxWidth
    property bool limitHeight: false

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
        room: r.room_
        eventId: r.eventId
        replyTo: ""
        mainInset: 4 + Nheko.paddingMedium
        maxWidth: r.maxWidth
        limitAsReply: r.limitHeight

        //height: replyContainer.implicitHeight
        data: Row {
            id: replyContainer

            spacing: Nheko.paddingSmall

            clip: r.limitHeight

            height: Math.min( timelineEvent.main?.height, timelineView.height / 10) + Nheko.paddingSmall + usernameBtn.height

            Rectangle {
                id: colorline

                width: 4
                height: content.height

                color: TimelineManager.userColor(r.userId, palette.base)
            }

            Column {
                id: content
                spacing: 0

                AbstractButton {
                    id: usernameBtn


                    contentItem: Label {
                        id: userName_
                        text: r.userName
                        color: r.userColor
                        textFormat: Text.RichText
                        width: timelineEvent.main?.width
                    }
                    onClicked: room.openUserProfile(r.userId)
                }

                data: [
                    usernameBtn, timelineEvent.main,
                ]
            }

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
