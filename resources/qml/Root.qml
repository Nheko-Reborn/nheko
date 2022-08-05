// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates"
import "./device-verification"
import "./dialogs"
import "./emoji"
import "./pages"
import "./voip"
import "./ui"
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import QtQuick.Window 2.15
import im.nheko 1.0
import im.nheko.EmojiModel 1.0

Pane {
    id: timelineRoot

    palette: Nheko.colors
    background: null
    padding: 0

    FontMetrics {
        id: fontMetrics
    }

    RoomDirectoryModel {
        id: publicRooms
    }

    //Timer {
    //    onTriggered: gc()
    //    interval: 1000
    //    running: true
    //    repeat: true
    //}

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

    function showAliasEditor(settings) {
        var dialog = aliasEditor.createObject(timelineRoot, {
            "roomSettings": settings
        });
        dialog.show();
        destroyOnClose(dialog);
    }

    Component {
        id: aliasEditor

        AliasEditor {
        }
    }

    function showPLEditor(settings) {
        var dialog = plEditor.createObject(timelineRoot, {
            "roomSettings": settings
        });
        dialog.show();
        destroyOnClose(dialog);
    }

    Component {
        id: plEditor

        PowerLevelEditor {
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

    Component {
        id: joinRoomDialog

        JoinRoomDialog {
        }

    }

    Component {
        id: confirmJoinRoomDialog

        ConfirmJoinRoomDialog {
        }

    }

    Component {
        id: leaveRoomComponent

        LeaveRoomDialog {
        }

    }

    Component {
        id: imageOverlay

        ImageOverlay {
        }

    }

    Component {
        id: userSettingsPage

        UserSettingsPage {
        }

    }

    Shortcut {
        sequence: StandardKey.Quit
        onActivated: Qt.quit()
    }

    Shortcut {
        sequence: "Ctrl+K"
        onActivated: {
            var quickSwitch = quickSwitcherComponent.createObject(timelineRoot);
            quickSwitch.open();
            destroyOnClosed(quickSwitch);
        }
    }

    Shortcut {
        // Add alternative shortcut, because sometimes Alt+A is stolen by the TextEdit
        sequences: ["Alt+A", "Ctrl+Shift+A"]
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
            destroyOnClose(dialog);
        }

        function onOpenJoinRoomDialog() {
            var dialog = joinRoomDialog.createObject(timelineRoot);
            dialog.show();
            destroyOnClose(dialog);
        }

        function onShowRoomJoinPrompt(summary) {
            var dialog = confirmJoinRoomDialog.createObject(timelineRoot, {"summary": summary});
            dialog.show();
            destroyOnClose(dialog);
        }

        target: Nheko
    }

    Connections {
        function onNewDeviceVerificationRequest(flow) {
            var dialog = deviceVerificationDialog.createObject(timelineRoot, {
                "flow": flow
            });
            dialog.show();
            destroyOnClose(dialog);
        }

        target: VerificationManager
    }

    function destroyOnClose(obj) {
        if (obj.closing != undefined) obj.closing.connect(() => obj.destroy(1000));
        else if (obj.aboutToHide != undefined) obj.aboutToHide.connect(() => obj.destroy(1000));
    }

    function destroyOnClosed(obj) {
        obj.aboutToHide.connect(() => obj.destroy(1000));
    }

    Connections {
        function onOpenProfile(profile) {
            var userProfile = userProfileComponent.createObject(timelineRoot, {
                "profile": profile
            });
            userProfile.show();
            destroyOnClose(userProfile);
        }

        function onShowImagePackSettings(room, packlist) {
            var packSet = packSettingsComponent.createObject(timelineRoot, {
                "room": room,
                "packlist": packlist
            });
            packSet.show();
            destroyOnClose(packSet);
        }

        function onOpenRoomMembersDialog(members, room) {
            var membersDialog = roomMembersComponent.createObject(timelineRoot, {
                "members": members,
                "room": room
            });
            membersDialog.show();
            destroyOnClose(membersDialog);
        }

        function onOpenRoomSettingsDialog(settings) {
            var roomSettings = roomSettingsComponent.createObject(timelineRoot, {
                "roomSettings": settings
            });
            roomSettings.show();
            destroyOnClose(roomSettings);
        }

        function onOpenInviteUsersDialog(invitees) {
            var dialog = inviteDialog.createObject(timelineRoot, {
                "roomId": Rooms.currentRoom.roomId,
                "plainRoomName": Rooms.currentRoom.plainRoomName,
                "invitees": invitees
            });
            dialog.show();
            destroyOnClose(dialog);
        }

        function onOpenLeaveRoomDialog(roomid, reason) {
            var dialog = leaveRoomComponent.createObject(timelineRoot, {
                "roomId": roomid,
                "reason": reason
            });
            dialog.open();
            destroyOnClose(dialog);
        }

        function onShowImageOverlay(room, eventId, url, originalWidth, proportionalHeight) {
            var dialog = imageOverlay.createObject(timelineRoot, {
                "room": room,
                "eventId": eventId,
                "url": url,
                "originalWidth": originalWidth ?? 0,
                "proportionalHeight": proportionalHeight ?? 0
            });

            dialog.showFullScreen();
            destroyOnClose(dialog);
        }

        target: TimelineManager
    }

    Connections {
        function onNewInviteState() {
            if (CallManager.haveCallInvite && Settings.mobileMode) {
                var dialog = mobileCallInviteDialog.createObject(timelineRoot);
                dialog.open();
                destroyOnClose(dialog);
            }
        }

        target: CallManager
    }

    SelfVerificationCheck {
    }

    InputDialog {
        id: uiaPassPrompt

        echoMode: TextInput.Password
        title: UIA.title
        prompt: qsTr("Please enter your login password to continue:")
        onAccepted: (t) => {
            return UIA.continuePassword(t);
        }
    }

    InputDialog {
        id: uiaEmailPrompt

        title: UIA.title
        prompt: qsTr("Please enter a valid email address to continue:")
        onAccepted: (t) => {
            return UIA.continueEmail(t);
        }
    }

    PhoneNumberInputDialog {
        id: uiaPhoneNumberPrompt

        title: UIA.title
        prompt: qsTr("Please enter a valid phone number to continue:")
        onAccepted: (p, t) => {
            return UIA.continuePhoneNumber(p, t);
        }
    }

    InputDialog {
        id: uiaTokenPrompt

        title: UIA.title
        prompt: qsTr("Please enter the token, which has been sent to you:")
        onAccepted: (t) => {
            return UIA.submit3pidToken(t);
        }
    }

    Platform.MessageDialog {
        id: uiaErrorDialog

        buttons: Platform.MessageDialog.Ok
    }

    Platform.MessageDialog {
        id: uiaConfirmationLinkDialog

        buttons: Platform.MessageDialog.Ok
        text: qsTr("Wait for the confirmation link to arrive, then continue.")
        onAccepted: UIA.continue3pidReceived()
    }

    Connections {
        function onPassword() {
            console.log("UIA: password needed");
            uiaPassPrompt.show();
        }

        function onEmail() {
            uiaEmailPrompt.show();
        }

        function onPhoneNumber() {
            uiaPhoneNumberPrompt.show();
        }

        function onPrompt3pidToken() {
            uiaTokenPrompt.show();
        }

        function onConfirm3pidToken() {
            uiaConfirmationLinkDialog.open();
        }

        function onError(msg) {
            uiaErrorDialog.text = msg;
            uiaErrorDialog.open();
        }

        target: UIA
    }

    StackView {
        id: mainWindow

        anchors.fill: parent
        initialItem: welcomePage
    }

    Component {
        id: welcomePage

        WelcomePage {
        }
    }

    Component {
        id: chatPage

        ChatPage {
        }
    }

    Component {
        id: loginPage

        LoginPage {
        }
    }

    Component {
        id: registerPage

        RegisterPage {
        }
    }

    Snackbar { id: snackbar }

    Connections {
        function onSwitchToChatPage() {
            mainWindow.replace(null, chatPage);
        }
        function onSwitchToLoginPage(error) {
            mainWindow.replace(welcomePage, {}, loginPage, {"error": error}, StackView.PopTransition);
        }
        function onShowNotification(msg) {
            snackbar.showNotification(msg);
            console.log("New snack: " + msg);
        }
        target: MainWindow
    }

}
