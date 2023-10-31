// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../ui"
import "../components"
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.13
import im.nheko 1.0

ApplicationWindow {
    id: roomMembersRoot

    property MemberList members
    property Room room

    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 650
    minimumHeight: 420
    title: qsTr("Members of %1").arg(members.roomName)
    width: 420

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok

        onAccepted: roomMembersRoot.close()
    }

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

            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 130
            Layout.preferredWidth: 130
            displayName: members.roomName
            roomid: members.roomId
            url: members.avatarUrl.replace("mxc://", "image://MxcImage/")

            onClicked: TimelineManager.openRoomSettings(members.roomId)
        }
        ElidedLabel {
            Layout.alignment: Qt.AlignHCenter
            elideWidth: parent.width - Nheko.paddingMedium
            font.pixelSize: fontMetrics.font.pixelSize * 2
            fullText: qsTr("%n people in %1", "Summary above list of members", members.memberCount).arg(members.roomName)
        }
        ImageButton {
            Layout.alignment: Qt.AlignHCenter
            ToolTip.text: qsTr("Invite more people")
            ToolTip.visible: hovered
            hoverEnabled: true
            image: ":/icons/icons/ui/add-square-button.svg"

            onClicked: TimelineManager.openInviteUsers(members.roomId)
        }
        MatrixTextField {
            id: searchBar

            Layout.fillWidth: true
            placeholderText: qsTr("Search...")

            Component.onCompleted: forceActiveFocus()
            onTextChanged: members.setFilterString(text)
        }
        RowLayout {
            spacing: Nheko.paddingMedium

            Label {
                color: palette.text
                text: qsTr("Sort by: ")
            }
            ComboBox {
                Layout.fillWidth: true
                textRole: "text"
                valueRole: "data"

                model: ListModel {
                    ListElement {
                        data: MemberList.Mxid
                        text: qsTr("User ID")
                    }
                    ListElement {
                        data: MemberList.DisplayName
                        text: qsTr("Display name")
                    }
                    ListElement {
                        data: MemberList.Powerlevel
                        text: qsTr("Power level")
                    }
                }

                onCurrentValueChanged: members.sortBy(currentValue)
            }
        }
        ScrollView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 200
            ScrollBar.horizontal.visible: false
            padding: Nheko.paddingMedium

            ListView {
                id: memberList

                boundsBehavior: Flickable.StopAtBounds
                clip: true
                model: members

                delegate: ItemDelegate {
                    id: del

                    height: memberLayout.implicitHeight + Nheko.paddingSmall * 2
                    hoverEnabled: true
                    padding: Nheko.paddingMedium
                    width: ListView.view.width

                    background: Rectangle {
                        color: del.hovered ? palette.dark : roomMembersRoot.color
                    }

                    onClicked: room.openUserProfile(model.mxid)

                    RowLayout {
                        id: memberLayout

                        anchors.centerIn: parent
                        spacing: Nheko.paddingMedium
                        width: parent.width - Nheko.paddingSmall * 2

                        Avatar {
                            id: avatar

                            Layout.preferredHeight: Nheko.avatarSize
                            Layout.preferredWidth: Nheko.avatarSize
                            displayName: model.displayName
                            enabled: false
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            userid: model.mxid
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Nheko.paddingSmall

                            ElidedLabel {
                                Layout.fillWidth: true
                                color: TimelineManager.userColor(model ? model.mxid : "", del.background.color)
                                elideWidth: del.width - Nheko.paddingMedium * 2 - avatar.width - encryptInd.width
                                font.pixelSize: fontMetrics.font.pixelSize
                                fullText: model.displayName
                            }
                            ElidedLabel {
                                Layout.fillWidth: true
                                color: del.hovered ? palette.brightText : palette.buttonText
                                elideWidth: del.width - Nheko.paddingMedium * 2 - avatar.width - encryptInd.width
                                font.pixelSize: Math.ceil(fontMetrics.font.pixelSize * 0.9)
                                fullText: model.mxid
                            }
                        }
                        PowerlevelIndicator {
                            permissions: room.permissions
                            powerlevel: model.powerlevel
                        }
                        EncryptionIndicator {
                            id: encryptInd

                            Layout.alignment: Qt.AlignRight
                            Layout.preferredHeight: 16
                            Layout.preferredWidth: 16
                            ToolTip.text: {
                                if (!encrypted)
                                    return qsTr("This room is not encrypted!");
                                switch (trust) {
                                case Crypto.Verified:
                                    return qsTr("This user is verified.");
                                case Crypto.TOFU:
                                    return qsTr("This user isn't verified, but is still using the same master key from the first time you met.");
                                default:
                                    return qsTr("This user has unverified devices!");
                                }
                            }
                            encrypted: room.isEncrypted
                            trust: encrypted ? model.trustlevel : Crypto.Unverified
                            visible: room.isEncrypted
                        }
                    }
                    NhekoCursorShape {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                    }
                }
                footer: Item {
                    anchors.margins: Nheko.paddingMedium
                    // use the default height if it's visible, otherwise no height at all
                    height: membersLoadingSpinner.implicitHeight
                    visible: (members.numUsersLoaded < members.memberCount) && members.loadingMoreMembers
                    width: parent.width

                    Spinner {
                        id: membersLoadingSpinner

                        anchors.centerIn: parent
                        implicitHeight: parent.visible ? 35 : 0
                    }
                }
            }
        }
    }
}
