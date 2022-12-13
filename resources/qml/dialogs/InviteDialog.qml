// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
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
    minimumWidth: 500

    Component.onCompleted: {
        friendsCompleter = TimelineManager.completerFor("user", "friends")
    }

    function addInvite(mxid) {
        if (mxid.match("@.+?:.{3,}")) {
            invitees.addUser(mxid);
            if (mxid == inviteeEntry.text)
                inviteeEntry.clear();
        } else
            console.log("invalid mxid: " + mxid)
    }

    function cleanUpAndClose() {
        if (inviteeEntry.isValidMxid)
            addInvite(inviteeEntry.text);

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

        Label {
            text: qsTr("User ID to invite")
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
                    if (isValidMxid)
                        addInvite(text);

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

            CheckBox {
                id: searchOnServer
                text: qsTr("Search on Server")
                checked: false
                onClicked: userSearch.model.setSearchString(inviteeEntry.text)
            }

        }
        RowLayout {
            ItemDelegate {
                visible: inviteeEntry.isValidMxid
                id: del3
                Layout.preferredWidth: inviteDialogRoot.width/2
                Layout.alignment: Qt.AlignTop
                Layout.preferredHeight: layout3.implicitHeight + Nheko.paddingSmall * 2
                onClicked: addInvite(inviteeEntry.text)
                background: Rectangle {
                color: del3.hovered ? Nheko.colors.dark : inviteDialogRoot.color
                clip: true
                }
                GridLayout {
                    id: layout3
                    anchors.centerIn: parent
                    width: del3.width - Nheko.paddingSmall * 2
                    rows: 2
                    columns: 2
                    rowSpacing: Nheko.paddingSmall
                    columnSpacing: Nheko.paddingMedium

                    Avatar {
                        Layout.rowSpan: 2
                        Layout.preferredWidth: Nheko.avatarSize
                        Layout.preferredHeight: Nheko.avatarSize
                        Layout.alignment: Qt.AlignLeft
                        userid: inviteeEntry.text
                        url: profile? profile.avatarUrl.replace("mxc://", "image://MxcImage/") : ""
                        displayName: profile? profile.displayName : ""
                        enabled: false
                    }
                    Label {
                        Layout.fillWidth: true
                        text: profile? profile.displayName : ""
                        color: TimelineManager.userColor(inviteeEntry.text, Nheko.colors.window)
                        font.pointSize: fontMetrics.font.pointSize
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        text: inviteeEntry.text
                        color: Nheko.colors.buttonText
                        font.pointSize: fontMetrics.font.pointSize * 0.9
                    }
                }
            }
            ListView {
                visible: !inviteeEntry.isValidMxid
                id: userSearch
                model: searchOnServer.checked? userDirectory : friendsCompleter
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                delegate: ItemDelegate {
                    id: del2
                    width: ListView.view.width
                    height: layout2.implicitHeight + Nheko.paddingSmall * 2
                    onClicked: addInvite(model.userid)
                    background: Rectangle {
                    color: del2.hovered ? Nheko.colors.dark : inviteDialogRoot.color
                    }
                    GridLayout {
                        id: layout2
                        anchors.centerIn: parent
                        width: del2.width - Nheko.paddingSmall * 2
                        rows: 2
                        columns: 2
                        rowSpacing: Nheko.paddingSmall
                        columnSpacing: Nheko.paddingMedium

                        Avatar {
                            Layout.rowSpan: 2
                            Layout.preferredWidth: Nheko.avatarSize
                            Layout.preferredHeight: Nheko.avatarSize
                            Layout.alignment: Qt.AlignLeft
                            userid: model.userid
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            displayName: model.displayName
                            enabled: false
                        }
                        Label {
                            Layout.fillWidth: true
                            text: model.displayName
                            color: TimelineManager.userColor(model.userid, Nheko.colors.window)
                            font.pointSize: fontMetrics.font.pointSize
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            text: model.userid
                            color: Nheko.colors.buttonText
                            font.pointSize: fontMetrics.font.pointSize * 0.9
                        }
                    }
                }
            }
            ListView {
                id: inviteesList

                Layout.fillWidth: true
                Layout.fillHeight: true
                model: invitees
                clip: true

                delegate: ItemDelegate {
                    id: del

                    hoverEnabled: true
                    width: ListView.view.width
                    height: layout.implicitHeight + Nheko.paddingSmall * 2
                    onClicked: TimelineManager.openGlobalUserProfile(model.mxid)
                    background: Rectangle {
                        color: del.hovered ? Nheko.colors.dark : inviteDialogRoot.color
                    }
                    GridLayout {
                        id: layout
                        anchors.centerIn: parent
                        width: del.width - Nheko.paddingSmall * 2
                        rows: 2
                        columns: 3
                        rowSpacing: Nheko.paddingSmall
                        columnSpacing: Nheko.paddingMedium

                        Avatar {
                            Layout.rowSpan: 2
                            Layout.preferredWidth: Nheko.avatarSize
                            Layout.preferredHeight: Nheko.avatarSize
                            Layout.alignment: Qt.AlignLeft
                            userid: model.mxid
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            displayName: model.displayName
                            enabled: false
                        }
                        Label {
                            Layout.fillWidth: true
                            text: model.displayName
                            color: TimelineManager.userColor(model.mxid, Nheko.colors.window)
                            font.pointSize: fontMetrics.font.pointSize
                        }

                        ImageButton {
                            Layout.rowSpan: 2
                            id: removeButton
                            image: ":/icons/icons/ui/dismiss.svg"
                            onClicked: invitees.removeUser(model.mxid)
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            text: model.mxid
                            color: Nheko.colors.buttonText
                            font.pointSize: fontMetrics.font.pointSize * 0.9
                        }

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
