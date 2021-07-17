// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import im.nheko 1.0
import "./ui"

ApplicationWindow {
    id: roomMembersRoot

    property MemberList members

    title: qsTr("Members of ") + members.roomName
    x: MainWindow.x + (MainWindow.width / 2) - (width / 2)
    y: MainWindow.y + (MainWindow.height / 2) - (height / 2)
    height: 650
    width: 420
    minimumHeight: 420

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: roomMembersRoot.close()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Avatar {
            id: roomAvatar

            width: 130
            height: width
            displayName: members.roomName
            Layout.alignment: Qt.AlignHCenter
            url: members.avatarUrl.replace("mxc://", "image://MxcImage/")
            onClicked: Rooms.currentRoom.openRoomSettings(members.roomId)
        }

        Label {
            font.pixelSize: 24
            text: members.memberCount + (members.memberCount === 1 ? qsTr(" person in ") : qsTr(" people in ")) + members.roomName
            Layout.alignment: Qt.AlignHCenter
        }

        ImageButton {
            Layout.alignment: Qt.AlignHCenter
            image: ":/icons/icons/ui/add-square-button.png"
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Invite more people")
            onClicked: Rooms.currentRoom.openInviteUsersDialog()
        }

        ScrollView {
            clip: false
            palette: Nheko.colors
            padding: 10
            ScrollBar.horizontal.visible: false
            Layout.fillHeight: true
            Layout.minimumHeight: 200
            Layout.fillWidth: true

            ListView {
                id: memberList

                clip: true
                spacing: 8
                boundsBehavior: Flickable.StopAtBounds
                model: members

                ScrollHelper {
                    flickable: parent
                    anchors.fill: parent
                    enabled: !Settings.mobileMode
                }

                delegate: RowLayout {
                    spacing: 10

                    Avatar {
                        width: Nheko.avatarSize
                        height: Nheko.avatarSize
                        userid: model.mxid
                        url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                        displayName: model.displayName
                        onClicked: Rooms.currentRoom.openUserProfile(model.mxid)
                    }

                    ColumnLayout {
                        spacing: 5

                        Label {
                            text: model.displayName
                            color: TimelineManager.userColor(model ? model.mxid : "", Nheko.colors.window)
                            font.pointSize: 12
                        }

                        Label {
                            text: model.mxid
                            color: Nheko.colors.buttonText
                            font.pointSize: 10
                        }

                        Item {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                        }
                    }
                }

                footer: Spinner {
                    // This is not a wonderful solution, but it is the best way to calculate whether
                    // all users are loaded while keeping canFetchMore() const

                    // TODO: just toggling the visiblity leaves some large empty space at the bottom
                    // of the list. This should be fixed.
                    visible: members.numUsersLoaded < members.memberCount
                    anchors.centerIn: parent
                }
            }
        }
    }

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok
        onAccepted: roomMembersRoot.close()
    }
}
