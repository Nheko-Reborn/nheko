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

    title: qsTr("Members of %1").arg(members.roomName)
    height: 650
    width: 420
    minimumHeight: 420
    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

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

            Layout.preferredHeight: 130
            Layout.preferredWidth: 130

            roomid: members.roomId
            displayName: members.roomName
            Layout.alignment: Qt.AlignHCenter
            url: members.avatarUrl.replace("mxc://", "image://MxcImage/")
            onClicked: TimelineManager.openRoomSettings(members.roomId)
        }

        ElidedLabel {
            font.pixelSize: fontMetrics.font.pixelSize * 2
            fullText: qsTr("%n people in %1", "Summary above list of members", members.memberCount).arg(members.roomName)
            Layout.alignment: Qt.AlignHCenter
            elideWidth: parent.width - Nheko.paddingMedium
        }

        ImageButton {
            Layout.alignment: Qt.AlignHCenter
            image: ":/icons/icons/ui/add-square-button.svg"
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Invite more people")
            onClicked: TimelineManager.openInviteUsers(members.roomId)
        }

        MatrixTextField {
            id: searchBar

            Layout.fillWidth: true
            placeholderText: qsTr("Search...")
            onTextChanged: members.setFilterString(text)

            Component.onCompleted: forceActiveFocus()
        }

        RowLayout {
            spacing: Nheko.paddingMedium

            Label {
                text: qsTr("Sort by: ")
                color: palette.text
            }

            ComboBox {
                model: ListModel {
                    ListElement { data: MemberList.Mxid; text: qsTr("User ID") }
                    ListElement { data: MemberList.DisplayName; text: qsTr("Display name") }
                    ListElement { data: MemberList.Powerlevel; text: qsTr("Power level") }
                }
                textRole: "text"
                valueRole: "data"
                onCurrentValueChanged: members.sortBy(currentValue)
                Layout.fillWidth: true
            }
        }

        ScrollView {
            padding: Nheko.paddingMedium
            ScrollBar.horizontal.visible: false
            Layout.fillHeight: true
            Layout.minimumHeight: 200
            Layout.fillWidth: true

            ListView {
                id: memberList

                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: members


                delegate: ItemDelegate {
                    id: del

                    onClicked: room.openUserProfile(model.mxid)
                    padding: Nheko.paddingMedium
                    width: ListView.view.width
                    height: memberLayout.implicitHeight + Nheko.paddingSmall * 2
                    hoverEnabled: true
                    background: Rectangle {
                        color: del.hovered ? palette.dark : roomMembersRoot.color
                    }

                    RowLayout {
                        id: memberLayout

                        spacing: Nheko.paddingMedium
                        anchors.centerIn: parent
                        width: parent.width - Nheko.paddingSmall * 2

                        Avatar {
                            id: avatar

                            Layout.preferredWidth: Nheko.avatarSize
                            Layout.preferredHeight: Nheko.avatarSize
                            userid: model.mxid
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            displayName: model.displayName
                            enabled: false
                        }

                        ColumnLayout {
                            spacing: Nheko.paddingSmall
                            Layout.fillWidth: true

                            ElidedLabel {
                                fullText: model.displayName
                                color: TimelineManager.userColor(model ? model.mxid : "", del.background.color)
                                font.pixelSize: fontMetrics.font.pixelSize
                                elideWidth: del.width - Nheko.paddingMedium * 2 - avatar.width - encryptInd.width
                                Layout.fillWidth: true
                            }

                            ElidedLabel {
                                fullText: model.mxid
                                color: del.hovered ? palette.brightText : palette.buttonText
                                font.pixelSize: Math.ceil(fontMetrics.font.pixelSize * 0.9)
                                elideWidth: del.width - Nheko.paddingMedium * 2 - avatar.width - encryptInd.width
                                Layout.fillWidth: true
                            }

                        }

                        PowerlevelIndicator {
                            Layout.preferredWidth: fontMetrics.lineSpacing * 2
                            Layout.preferredHeight: fontMetrics.lineSpacing * 2
                            sourceSize.width: width
                            sourceSize.height: height
                            powerlevel: model.powerlevel
                            permissions: room.permissions
                        }

                        EncryptionIndicator {
                            id: encryptInd

                            Layout.preferredWidth: fontMetrics.lineSpacing * 2
                            Layout.preferredHeight: fontMetrics.lineSpacing * 2
                            sourceSize.width: width
                            sourceSize.height: height
                            Layout.alignment: Qt.AlignRight
                            visible: room.isEncrypted
                            encrypted: room.isEncrypted
                            trust: encrypted ? model.trustlevel : Crypto.Unverified
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
                        }

                    }

                    NhekoCursorShape {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                    }

                }

                footer: Item {
                    width: parent.width
                    visible: (members.numUsersLoaded < members.memberCount) && members.loadingMoreMembers
                    // use the default height if it's visible, otherwise no height at all
                    height: membersLoadingSpinner.implicitHeight
                    anchors.margins: Nheko.paddingMedium

                    Spinner {
                        id: membersLoadingSpinner

                        anchors.centerIn: parent
                        implicitHeight: parent.visible ? 35 : 0
                    }

                }

            }

        }

    }

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok
        onAccepted: roomMembersRoot.close()
    }

}
