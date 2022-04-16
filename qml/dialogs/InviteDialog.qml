// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import im.nheko

ApplicationWindow {
    id: inviteDialogRoot

    property InviteesModel invitees
    property string plainRoomName
    property string roomId

    function addInvite() {
        if (inviteeEntry.isValidMxid) {
            invitees.addUser(inviteeEntry.text);
            inviteeEntry.clear();
        }
    }
    function cleanUpAndClose() {
        if (inviteeEntry.isValidMxid)
            addInvite();
        invitees.accept();
        close();
    }

    color: timelineRoot.palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 380
    palette: timelineRoot.palette
    title: qsTr("Invite users to %1").arg(plainRoomName)
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
            Layout.fillWidth: true
            color: timelineRoot.palette.text
            text: qsTr("User ID to invite")
        }
        RowLayout {
            spacing: Nheko.paddingMedium

            MatrixTextField {
                id: inviteeEntry

                property bool isValidMxid: text.match("@.+?:.{3,}")

                Layout.fillWidth: true
                backgroundColor: timelineRoot.palette.window
                placeholderText: qsTr("@joe:matrix.org", "Example user id. The name 'joe' can be localized however you want.")

                Component.onCompleted: forceActiveFocus()
                Keys.onPressed: {
                    if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers === Qt.ControlModifier))
                        cleanUpAndClose();
                }
                Keys.onShortcutOverride: event.accepted = ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers & Qt.ControlModifier))
                onAccepted: {
                    if (isValidMxid)
                        addInvite();
                }
            }
            Button {
                enabled: inviteeEntry.isValidMxid
                text: qsTr("Add")

                onClicked: addInvite()
            }
        }
        ListView {
            id: inviteesList
            Layout.fillHeight: true
            Layout.fillWidth: true
            model: invitees

            delegate: ItemDelegate {
                id: del
                height: layout.implicitHeight + Nheko.paddingSmall * 2
                hoverEnabled: true
                width: ListView.view.width

                background: Rectangle {
                    color: del.hovered ? timelineRoot.palette.dark : inviteDialogRoot.color
                }

                onClicked: TimelineManager.openGlobalUserProfile(model.mxid)

                RowLayout {
                    id: layout
                    anchors.centerIn: parent
                    spacing: Nheko.paddingMedium
                    width: del.width - Nheko.paddingSmall * 2

                    Avatar {
                        displayName: model.displayName
                        enabled: false
                        height: Nheko.avatarSize
                        url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                        userid: model.mxid
                        width: Nheko.avatarSize
                    }
                    ColumnLayout {
                        spacing: Nheko.paddingSmall

                        Label {
                            color: TimelineManager.userColor(model ? model.mxid : "", del.background.color)
                            font.pointSize: fontMetrics.font.pointSize
                            text: model.displayName
                        }
                        Label {
                            color: del.hovered ? timelineRoot.palette.brightText : timelineRoot.palette.placeholderText
                            font.pointSize: fontMetrics.font.pointSize * 0.9
                            text: model.mxid
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ImageButton {
                        image: ":/icons/icons/ui/dismiss.svg"

                        onClicked: invitees.removeUser(model.mxid)
                    }
                }
                NhekoCursorShape {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
    }
}
