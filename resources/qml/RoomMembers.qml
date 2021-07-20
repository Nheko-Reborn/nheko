// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./ui"
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import im.nheko 1.0

ApplicationWindow {
    id: roomMembersRoot

    property MemberList members

    title: qsTr("Members of ") + members.roomName
    x: MainWindow.x + (MainWindow.width / 2) - (width / 2)
    y: MainWindow.y + (MainWindow.height / 2) - (height / 2)
    height: 650
    width: 420
    minimumHeight: 420
    palette: Nheko.colors
    color: Nheko.colors.window

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: roomMembersRoot.close()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Avatar {
            id: roomAvatar

            width: 130
            height: width
            displayName: members.roomName
            Layout.alignment: Qt.AlignHCenter
            url: members.avatarUrl.replace("mxc://", "image://MxcImage/")
            onClicked: Rooms.currentRoom.openRoomSettings(members.roomId)
        }

        ElidedLabel {
            font.pixelSize: fontMetrics.font.pixelSize * 2
            fullText: members.memberCount + (members.memberCount === 1 ? qsTr(" person in ") : qsTr(" people in ")) + members.roomName
            Layout.alignment: Qt.AlignHCenter
        }

        ImageButton {
            Layout.alignment: Qt.AlignHCenter
            image: ":/icons/icons/ui/add-square-button.png"
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Invite more people")
            onClicked: Rooms.currentRoom.openInviteUsers()
        }

        ScrollView {
            palette: Nheko.colors
            padding: Nheko.paddingMedium
            ScrollBar.horizontal.visible: false
            Layout.fillHeight: true
            Layout.minimumHeight: 200
            Layout.fillWidth: true

            ListView {
                id: memberList

                clip: true
                spacing: Nheko.paddingMedium
                boundsBehavior: Flickable.StopAtBounds
                model: members

                ScrollHelper {
                    flickable: parent
                    anchors.fill: parent
                    enabled: !Settings.mobileMode
                }

                delegate: RowLayout {
                    spacing: Nheko.paddingMedium

                    Avatar {
                        width: Nheko.avatarSize
                        height: Nheko.avatarSize
                        userid: model.mxid
                        url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                        displayName: model.displayName
                        onClicked: Rooms.currentRoom.openUserProfile(model.mxid)
                    }

                    ColumnLayout {
                        spacing: Nheko.paddingSmall

                        Label {
                            text: model.displayName
                            color: TimelineManager.userColor(model ? model.mxid : "", Nheko.colors.window)
                            font.pointSize: fontMetrics.font.pointSize
                        }

                        Label {
                            text: model.mxid
                            color: Nheko.colors.buttonText
                            font.pointSize: fontMetrics.font.pointSize * 0.9
                        }

                        Item {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                        }

                    }

                }

                footer: Spinner {
                    visible: members.numUsersLoaded < members.memberCount && members.loadingMoreMembers
                    // use the default height if it's visible, otherwise no height at all
                    height: visible ? undefined : 0
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
