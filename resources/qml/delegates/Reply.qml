// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
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
    implicitWidth: replyContainer.implicitWidth + leftPadding + rightPadding

    leftPadding: 4 + Nheko.paddingSmall
    rightPadding: Nheko.paddingSmall

    required property int maxWidth
    property bool limitHeight: false

    NhekoCursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

    onClicked: {
        let link = timelineEvent.main.linkAt != undefined && timelineEvent.main.linkAt(pressX-colorline.width, pressY - userName_.implicitHeight);
        if (link) {
            Nheko.openLink(link)
        } else {
            room.showEvent(r.eventId)
        }
    }
    onPressAndHold: replyContextMenu.show(timelineEvent.main.copyText, timelineEvent.main.linkAt(pressX-colorline.width, pressY - userName_.implicitHeight), r.eventId)

    contentItem: TimelineEvent {
        id: timelineEvent

        isStateEvent: false
        room: r.room_
        eventId: r.eventId
        replyTo: ""
        mainInset: 4 + Nheko.paddingMedium
        maxWidth: r.maxWidth
        limitAsReply: r.limitHeight

        data: Column {
            id: replyContainer
            spacing: 0

            clip: r.limitHeight

            height: r.limitHeight ? Math.min( timelineEvent.main?.height, timelineView.height / 10) + Nheko.paddingSmall + usernameBtn.height : undefined

            // FIXME: I have no idea, why this name doesn't render in the reply popup on Qt 6.9.2
            AbstractButton {
                id: usernameBtn

                visible: r.eventId

                contentItem: Label {
                    visible: r.eventId
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

    background: Rectangle {
        id: backgroundItem

        z: -1
        property color userColor: TimelineManager.userColor(r.userId, palette.base)
        property color bgColor: palette.base
        color: Qt.tint(bgColor, Qt.hsla(userColor.hslHue, 0.5, userColor.hslLightness, 0.1))

        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left

            id: colorline
            color: backgroundItem.userColor
            width: 4
        }
    }

}
