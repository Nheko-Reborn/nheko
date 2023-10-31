// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../components"
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

ApplicationWindow {
    id: inviteDialogRoot

    property var friendsCompleter
    property InviteesModel invitees
    property var profile

    function addInvite(mxid, displayName, avatarUrl) {
        if (mxid.match("@.+?:.{3,}")) {
            invitees.addUser(mxid, displayName, avatarUrl);
        } else
            console.log("invalid mxid: " + mxid);
    }
    function cleanUpAndClose() {
        if (inviteeEntry.isValidMxid)
            addInvite(inviteeEntry.text, "", "");
        invitees.accept();
        close();
    }

    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 380
    minimumWidth: 300
    title: qsTr("Invite users to %1").arg(invitees.room.plainRoomName)
    width: 340

    footer: DialogButtonBox {
        id: buttons

        Button {
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            enabled: invitees.count > 0
            text: qsTr("Invite")

            onClicked: cleanUpAndClose()
        }
        Button {
            DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
            text: qsTr("Cancel")

            onClicked: inviteDialogRoot.close()
        }
    }

    Component.onCompleted: {
        friendsCompleter = TimelineManager.completerFor("user", "friends");
        width = 600;
    }

    Shortcut {
        sequence: "Ctrl+Enter"

        onActivated: cleanUpAndClose()
    }
    Shortcut {
        sequence: StandardKey.Cancel

        onActivated: inviteDialogRoot.close()
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            layoutDirection: Qt.LeftToRight
            spacing: 4
            visible: !inviteesList.visible

            Repeater {
                id: inviteesRepeater

                model: invitees

                delegate: ItemDelegate {
                    id: inviteeButton

                    background: Rectangle {
                        border.color: palette.text
                        border.width: 1
                        color: inviteeButton.hovered ? palette.highlight : palette.window
                        radius: inviteeButton.height / 2
                    }
                    contentItem: Label {
                        id: inviteeUserid

                        anchors.centerIn: parent
                        color: inviteeButton.hovered ? palette.highlightedText : palette.text
                        maximumLineCount: 1
                        text: model.displayName != "" ? model.displayName : model.userid
                    }

                    onClicked: invitees.removeUser(model.mxid)
                }
            }
        }
        Label {
            Layout.fillWidth: true
            color: palette.text
            text: qsTr("Search user")
        }
        RowLayout {
            spacing: Nheko.paddingMedium

            MatrixTextField {
                id: inviteeEntry

                property bool isValidMxid: text.match("@.+?:.{3,}")

                Layout.fillWidth: true
                backgroundColor: palette.window
                placeholderText: qsTr("@user:yourserver.example.com", "Example user id. The name 'user' can be localized however you want.")

                Component.onCompleted: forceActiveFocus()
                Keys.onPressed: {
                    if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers === Qt.ControlModifier))
                        cleanUpAndClose();
                }
                Keys.onShortcutOverride: event.accepted = ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers & Qt.ControlModifier))
                onAccepted: {
                    if (isValidMxid) {
                        addInvite(text, "", "");
                        clear();
                    } else if (userSearch.count > 0) {
                        addInvite(userSearch.itemAtIndex(0).userid, userSearch.itemAtIndex(0).displayName, userSearch.itemAtIndex(0).avatarUrl);
                        clear();
                    }
                }
                onTextChanged: {
                    searchTimer.restart();
                    if (isValidMxid) {
                        profile = TimelineManager.getGlobalUserProfile(text);
                    } else
                        profile = null;
                }

                Timer {
                    id: searchTimer

                    interval: 350

                    onTriggered: {
                        userSearch.model.setSearchString(parent.text);
                    }
                }
            }
            ToggleButton {
                id: searchOnServer

                checked: false

                onClicked: userSearch.model.setSearchString(inviteeEntry.text)
            }
            MatrixText {
                text: qsTr("Search on Server")
            }
        }
        RowLayout {
            UserListRow {
                id: del3

                Layout.alignment: Qt.AlignTop
                Layout.preferredHeight: implicitHeight
                Layout.preferredWidth: inviteDialogRoot.width / 2
                avatarUrl: profile ? profile.avatarUrl : ""
                bgColor: del3.hovered ? palette.dark : inviteDialogRoot.color
                displayName: profile ? profile.displayName : ""
                userid: inviteeEntry.text
                visible: inviteeEntry.isValidMxid

                onClicked: addInvite(inviteeEntry.text, displayName, avatarUrl)
            }
            ListView {
                id: userSearch

                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                model: searchOnServer.checked ? userDirectory : friendsCompleter
                visible: !inviteeEntry.isValidMxid

                delegate: UserListRow {
                    id: del2

                    avatarUrl: model.avatarUrl
                    bgColor: del2.hovered ? palette.dark : inviteDialogRoot.color
                    displayName: model.displayName
                    height: implicitHeight
                    userid: model.userid
                    width: ListView.view.width

                    onClicked: addInvite(userid, displayName, avatarUrl)
                }
            }
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                color: Nheko.theme.separator
                visible: inviteesList.visible
            }
            ListView {
                id: inviteesList

                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                model: invitees
                visible: inviteDialogRoot.width >= 500

                delegate: UserListRow {
                    id: del

                    avatarUrl: model.avatarUrl
                    bgColor: del.hovered ? palette.dark : inviteDialogRoot.color
                    displayName: model.displayName
                    height: implicitHeight
                    hoverEnabled: true
                    userid: model.mxid
                    width: ListView.view.width

                    onClicked: TimelineManager.openGlobalUserProfile(model.mxid)

                    ImageButton {
                        id: removeButton

                        anchors.right: parent.right
                        anchors.rightMargin: Nheko.paddingSmall
                        anchors.top: parent.top
                        anchors.topMargin: Nheko.paddingSmall
                        image: ":/icons/icons/ui/dismiss.svg"

                        onClicked: invitees.removeUser(model.mxid)
                    }
                    NhekoCursorShape {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }
        }
    }
}
