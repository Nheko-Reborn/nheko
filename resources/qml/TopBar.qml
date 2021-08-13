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

    property bool showBackButton: false
    property string roomName: room ? room.roomName : qsTr("No room selected")
    property string avatarUrl: room ? room.roomAvatarUrl : ""
    property string roomTopic: room ? room.roomTopic : ""
    property bool isEncrypted: room ? room.isEncrypted : false
    property int trustlevel: room ? room.trustlevel : Crypto.Unverified

    Layout.fillWidth: true
    implicitHeight: topLayout.height + Nheko.paddingMedium * 2
    z: 3
    color: Nheko.colors.window

    TapHandler {
        onSingleTapped: {
            if (room)
                TimelineManager.openRoomSettings(room.roomId);

            eventPoint.accepted = true;
        }
        gesturePolicy: TapHandler.ReleaseWithinBounds
    }

    GridLayout {
        id: topLayout

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Nheko.paddingMedium
        anchors.verticalCenter: parent.verticalCenter

        ImageButton {
            id: backToRoomsButton

            Layout.column: 0
            Layout.row: 0
            Layout.rowSpan: 2
            Layout.alignment: Qt.AlignVCenter
            width: Nheko.avatarSize
            height: Nheko.avatarSize
            visible: showBackButton
            image: ":/icons/icons/ui/angle-pointing-to-left.png"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Back to room list")
            onClicked: Rooms.resetCurrentRoom()
        }

        Avatar {
            Layout.column: 1
            Layout.row: 0
            Layout.rowSpan: 2
            Layout.alignment: Qt.AlignVCenter
            width: Nheko.avatarSize
            height: Nheko.avatarSize
            url: avatarUrl.replace("mxc://", "image://MxcImage/")
            displayName: roomName
            onClicked: {
                if (room)
                    TimelineManager.openRoomSettings(room.roomId);

            }
        }

        Label {
            Layout.fillWidth: true
            Layout.column: 2
            Layout.row: 0
            color: Nheko.colors.text
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: roomName
            maximumLineCount: 1
            elide: Text.ElideRight
            textFormat: Text.RichText
        }

        MatrixText {
            Layout.fillWidth: true
            Layout.column: 2
            Layout.row: 1
            Layout.maximumHeight: fontMetrics.lineSpacing * 2 // show 2 lines
            clip: true
            text: roomTopic
        }

        EncryptionIndicator {
            Layout.column: 3
            Layout.row: 0
            Layout.rowSpan: 2
            visible: isEncrypted
            encrypted: isEncrypted
            trust: trustlevel
            ToolTip.text: {
                if (!encrypted)
                    return qsTr("This room is not encrypted!");

                switch (trust) {
                case Crypto.Verified:
                    return qsTr("This room contains only verified devices.");
                case Crypto.TOFU:
                    return qsTr("This rooms contain verified devices and devices which have never changed their master key.");
                default:
                    return qsTr("This room contains unverified devices!");
                }
            }
        }

        ImageButton {
            id: roomOptionsButton

            visible: !!room
            Layout.column: 4
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
                    visible: room ? room.permissions.canInvite() : false
                    text: qsTr("Invite users")
                    onTriggered: TimelineManager.openInviteUsers(room.roomId)
                }

                Platform.MenuItem {
                    text: qsTr("Members")
                    onTriggered: TimelineManager.openRoomMembers(room)
                }

                Platform.MenuItem {
                    text: qsTr("Leave room")
                    onTriggered: TimelineManager.openLeaveRoomDialog(room.roomId)
                }

                Platform.MenuItem {
                    text: qsTr("Settings")
                    onTriggered: TimelineManager.openRoomSettings(room.roomId)
                }

            }

        }

    }

}
