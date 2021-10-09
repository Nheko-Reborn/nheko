// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates"
import "./device-verification"
import "./dialogs"
import "./emoji"
import "./voip"
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import QtQuick.Window 2.15
import im.nheko 1.0
import im.nheko.EmojiModel 1.0

Page {
    id: timelineRoot

    palette: Nheko.colors

    FontMetrics {
        id: fontMetrics
    }

    EmojiPicker {
        id: emojiPopup

        colors: palette
        model: TimelineManager.completerFor("allemoji", "")
    }

    Component {
        id: userProfileComponent

        UserProfile {
        }

    }

    Component {
        id: roomSettingsComponent

        RoomSettings {
        }

    }

    Component {
        id: roomMembersComponent

        RoomMembers {
        }

    }

    Component {
        id: mobileCallInviteDialog

        CallInvite {
        }

    }

    Component {
        id: quickSwitcherComponent

        QuickSwitcher {
        }

    }

    Component {
        id: deviceVerificationDialog

        DeviceVerification {
        }

    }

    Component {
        id: inviteDialog

        InviteDialog {
        }

    }

    Component {
        id: packSettingsComponent

        ImagePackSettingsDialog {
        }

    }

    Component {
        id: readReceiptsDialog

        ReadReceipts {
        }

    }

    Component {
        id: rawMessageDialog

        RawMessageDialog {
        }

    }

    Component {
        id: logoutDialog

        LogoutDialog {
        }
    }

    Shortcut {
        sequence: "Ctrl+K"
        onActivated: {
            var quickSwitch = quickSwitcherComponent.createObject(timelineRoot);
            TimelineManager.focusTimeline();
            quickSwitch.open();
        }
    }

    Shortcut {
        sequence: "Alt+A"
        onActivated: Rooms.nextRoomWithActivity()
    }

    Shortcut {
        sequence: "Ctrl+Down"
        onActivated: Rooms.nextRoom()
    }

    Shortcut {
        sequence: "Ctrl+Up"
        onActivated: Rooms.previousRoom()
    }

    Connections {
        function onOpenLogoutDialog() {
            var dialog = logoutDialog.createObject(timelineRoot);
            dialog.open();
        }

        target: Nheko
    }

    Connections {
        function onNewDeviceVerificationRequest(flow) {
            var dialog = deviceVerificationDialog.createObject(timelineRoot, {
                "flow": flow
            });
            dialog.show();
        }

        function onOpenProfile(profile) {
            var userProfile = userProfileComponent.createObject(timelineRoot, {
                "profile": profile
            });
            userProfile.show();
        }

        function onShowImagePackSettings(packlist) {
            var packSet = packSettingsComponent.createObject(timelineRoot, {
                "packlist": packlist
            });
            packSet.show();
        }

        function onOpenRoomMembersDialog(members, room) {
            var membersDialog = roomMembersComponent.createObject(timelineRoot, {
                "members": members,
                "room": room
            });
            membersDialog.show();
        }

        function onOpenRoomSettingsDialog(settings) {
            var roomSettings = roomSettingsComponent.createObject(timelineRoot, {
                "roomSettings": settings
            });
            roomSettings.show();
        }

        function onOpenInviteUsersDialog(invitees) {
            var dialog = inviteDialog.createObject(timelineRoot, {
                "roomId": Rooms.currentRoom.roomId,
                "plainRoomName": Rooms.currentRoom.plainRoomName,
                "invitees": invitees
            });
            dialog.show();
        }

        target: TimelineManager
    }

    Connections {
        function onNewInviteState() {
            if (CallManager.haveCallInvite && Settings.mobileMode) {
                var dialog = mobileCallInviteDialog.createObject(msgView);
                dialog.open();
            }
        }

        target: CallManager
    }

    ChatPage {
        anchors.fill: parent
    }

}
