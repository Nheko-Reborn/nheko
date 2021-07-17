// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates"
import "./device-verification"
import "./emoji"
import "./voip"
import Qt.labs.platform 1.1 as Platform
import QtGraphicalEffects 1.0
import QtQuick 2.9
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
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

    Shortcut {
        sequence: "Ctrl+K"
        onActivated: {
            var quickSwitch = quickSwitcherComponent.createObject(timelineRoot);
            TimelineManager.focusTimeline();
            quickSwitch.open();
        }
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
        target: TimelineManager
        onNewDeviceVerificationRequest: {
            var dialog = deviceVerificationDialog.createObject(timelineRoot, {
                "flow": flow
            });
            dialog.show();
        }
        onOpenProfile: {
            var userProfile = userProfileComponent.createObject(timelineRoot, {
                "profile": profile
            });
            userProfile.show();
        }
    }

    Connections {
        target: CallManager
        onNewInviteState: {
            if (CallManager.haveCallInvite && Settings.mobileMode) {
                var dialog = mobileCallInviteDialog.createObject(msgView);
                dialog.open();
            }
        }
    }

    Connections {
        target: Rooms.currentRoom
        onOpenRoomMembersDialog: {
            var membersDialog = roomMembersComponent.createObject(timelineRoot, {
                "members": members,
                "roomName": Rooms.currentRoom.roomName
            });
            membersDialog.show();
        }
    }

    Connections {
        target: Rooms.currentRoom
        onOpenRoomSettingsDialog: {
            var roomSettings = roomSettingsComponent.createObject(timelineRoot, {
                "roomSettings": settings
            });
            roomSettings.show();
        }
    }

    Connections {
        target: Rooms.currentRoom
        onOpenInviteUsersDialog: {
            var dialog = inviteDialog.createObject(timelineRoot, {
                "roomId": Rooms.currentRoom.roomId,
                "roomName": Rooms.currentRoom.roomName
            });
            dialog.show();
        }
    }

    ChatPage {
        anchors.fill: parent
    }

}
