// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../components"
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import im.nheko 1.0

ApplicationWindow {
    id: inviteDialogRoot

    property string roomId
    property string plainRoomName
    property InviteesModel invitees
    property var friendsCompleter
    property var profile
    minimumWidth: 300

    Component.onCompleted: {
        friendsCompleter = TimelineManager.completerFor("user", "friends")
        width = 600
    }

    function addInvite(mxid, displayName, avatarUrl) {
        if (mxid.match("@.+?:.{3,}")) {
            invitees.addUser(mxid, displayName, avatarUrl);
        } else
            console.log("invalid mxid: " + mxid)
    }

    function cleanUpAndClose() {
        if (inviteeEntry.isValidMxid)
            addInvite(inviteeEntry.text, "", "");

        invitees.accept();
        close();
    }

    title: qsTr("Invite users to %1").arg(plainRoomName)
    height: 380
    width: 340
    palette: Nheko.colors
    color: Nheko.colors.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

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
            layoutDirection: Qt.LeftToRight
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            spacing: 4
            visible: !inviteesList.visible
            Repeater {
                id: inviteesRepeater
                model: invitees
                delegate: ItemDelegate {
                    onClicked: invitees.removeUser(model.mxid)
                    id: inviteeButton
                    contentItem: Label {
                        anchors.centerIn: parent
                        id: inviteeUserid
                        text: model.displayName != "" ? model.displayName : model.userid
                        color: inviteeButton.hovered ? Nheko.colors.highlightedText: Nheko.colors.text
                        maximumLineCount: 1
                    }
                    background: Rectangle {
                        border.color: Nheko.colors.text
                        color: inviteeButton.hovered ? Nheko.colors.highlight : Nheko.colors.window
                        border.width: 1
                        radius: inviteeButton.height / 2
                    }
                }
            }
        }

        Label {
            text: qsTr("Search user")
            Layout.fillWidth: true
            color: Nheko.colors.text
        }
        RowLayout {
            spacing: Nheko.paddingMedium

            MatrixTextField {
                id: inviteeEntry

                property bool isValidMxid: text.match("@.+?:.{3,}")

                backgroundColor: Nheko.colors.window
                placeholderText: qsTr("@joe:matrix.org", "Example user id. The name 'joe' can be localized however you want.")
                Layout.fillWidth: true
                onAccepted: {
                    if (isValidMxid) {
                        addInvite(text, "", "");
                        clear()
                    }
                    else if (userSearch.count > 0) {
                        addInvite(userSearch.itemAtIndex(0).userid, userSearch.itemAtIndex(0).displayName, userSearch.itemAtIndex(0).avatarUrl)
                        clear()
                    }
                }
                Component.onCompleted: forceActiveFocus()
                Keys.onShortcutOverride: event.accepted = ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers & Qt.ControlModifier))
                Keys.onPressed: {
                    if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers === Qt.ControlModifier))
                        cleanUpAndClose();

                }
                onTextChanged: {
                    searchTimer.restart()
                    if(isValidMxid) {
                        profile = TimelineManager.getGlobalUserProfile(text);
                    } else
                        profile = null;
                }
                Timer {
                    id: searchTimer

                    interval: 350
                    onTriggered: {
                        userSearch.model.setSearchString(parent.text)
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
                visible: inviteeEntry.isValidMxid
                id: del3
                Layout.preferredWidth: inviteDialogRoot.width/2
                Layout.alignment: Qt.AlignTop
                Layout.preferredHeight: implicitHeight
                displayName: profile? profile.displayName : ""
                avatarUrl: profile? profile.avatarUrl : ""
                userid: inviteeEntry.text
                onClicked: addInvite(inviteeEntry.text, displayName, avatarUrl)
                bgColor: del3.hovered ? Nheko.colors.dark : inviteDialogRoot.color
            }
            ListView {
                visible: !inviteeEntry.isValidMxid
                id: userSearch
                model: searchOnServer.checked? userDirectory : friendsCompleter
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                delegate: UserListRow {
                    id: del2
                    width: ListView.view.width
                    height: implicitHeight
                    displayName: model.displayName
                    userid: model.userid
                    avatarUrl: model.avatarUrl
                    onClicked: addInvite(userid, displayName, avatarUrl)
                    bgColor: del2.hovered ? Nheko.colors.dark : inviteDialogRoot.color
                }
            }
            Rectangle {
                Layout.fillHeight: true
                visible: inviteesList.visible
                width: 1
                color: Nheko.theme.separator
            }
            ListView {
                id: inviteesList

                Layout.fillWidth: true
                Layout.fillHeight: true
                model: invitees
                clip: true
                visible: inviteDialogRoot.width >= 500

                delegate: UserListRow {
                    id: del
                    hoverEnabled: true
                    width: ListView.view.width
                    height: implicitHeight
                    onClicked: TimelineManager.openGlobalUserProfile(model.mxid)
                    userid: model.mxid
                    avatarUrl: model.avatarUrl
                    displayName: model.displayName
                    bgColor: del.hovered ? Nheko.colors.dark : inviteDialogRoot.color
                    ImageButton {
                        anchors.right: parent.right
                        anchors.rightMargin: Nheko.paddingSmall
                        anchors.top: parent.top
                        anchors.topMargin: Nheko.paddingSmall
                        id: removeButton
                        image: ":/icons/icons/ui/dismiss.svg"
                        onClicked: invitees.removeUser(model.mxid)
                    }

                    CursorShape {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                    }

                }

            }
        }

    }

    footer: DialogButtonBox {
        id: buttons

        Button {
            text: qsTr("Invite")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            enabled: invitees.count > 0
            onClicked: cleanUpAndClose()
        }

        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
            onClicked: inviteDialogRoot.close()
        }

    }

}
