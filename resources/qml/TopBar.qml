// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import Qt.labs.platform 1.1 as Platform
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.2
import im.nheko 1.0

Rectangle {
    id: topBar

    property var room: TimelineManager.timeline

    Layout.fillWidth: true
    implicitHeight: topLayout.height + 16
    z: 3
    color: colors.window

    TapHandler {
        onSingleTapped: TimelineManager.timeline.openRoomSettings()
    }

    GridLayout {
        //Layout.margins: 8

        id: topLayout

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        anchors.verticalCenter: parent.verticalCenter

        ImageButton {
            id: backToRoomsButton

            Layout.column: 0
            Layout.row: 0
            Layout.rowSpan: 2
            Layout.alignment: Qt.AlignVCenter
            width: avatarSize
            height: avatarSize
            visible: TimelineManager.isNarrowView
            image: ":/icons/icons/ui/angle-pointing-to-left.png"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Back to room list")
            onClicked: TimelineManager.backToRooms()
        }

        Avatar {
            Layout.column: 1
            Layout.row: 0
            Layout.rowSpan: 2
            Layout.alignment: Qt.AlignVCenter
            width: avatarSize
            height: avatarSize
            url: room ? room.roomAvatarUrl.replace("mxc://", "image://MxcImage/") : ""
            displayName: room ? room.roomName : qsTr("No room selected")
            onClicked: TimelineManager.openRoomSettings()
        }

        Label {
            Layout.fillWidth: true
            Layout.column: 2
            Layout.row: 0
            color: colors.text
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: room ? room.roomName : qsTr("No room selected")
            maximumLineCount: 1
            elide: Text.ElideRight

            TapHandler {
                onSingleTapped: TimelineManager.timeline.openRoomSettings()
            }

        }

        MatrixText {
            Layout.fillWidth: true
            Layout.column: 2
            Layout.row: 1
            Layout.maximumHeight: fontMetrics.lineSpacing * 2 // show 2 lines
            clip: true
            text: room ? room.roomTopic : ""
        }

        ImageButton {
            id: roomOptionsButton

            Layout.column: 3
            Layout.row: 0
            Layout.rowSpan: 2
            Layout.alignment: Qt.AlignVCenter
            image: ":/icons/icons/ui/vertical-ellipsis.png"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Room options")
            onClicked: roomOptionsMenu.open(roomOptionsButton)

            Platform.Menu {
                id: roomOptionsMenu

                Platform.MenuItem {
                    text: qsTr("Invite users")
                    onTriggered: TimelineManager.openInviteUsersDialog()
                }

                Platform.MenuItem {
                    text: qsTr("Members")
                    onTriggered: TimelineManager.openMemberListDialog()
                }

                Platform.MenuItem {
                    text: qsTr("Leave room")
                    onTriggered: TimelineManager.openLeaveRoomDialog()
                }

                Platform.MenuItem {
                    text: qsTr("Settings")
                    onTriggered: TimelineManager.timeline.openRoomSettings()
                }

            }

        }

    }

}
