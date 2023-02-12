// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
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

    UserDirectoryModel {
        id: userDirectory
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

    function showAliasEditor(settings) {
        var dialog = Qt.createComponent("qrc:/qml/dialogs/AliasEditor.qml").createObject(timelineRoot, {
            "roomSettings": settings
        });
        dialog.show();
        destroyOnClose(dialog);
    }

    function showPLEditor(settings) {
        var dialog = Qt.createComponent("qrc:/qml/dialogs/PowerLevelEditor.qml").createObject(timelineRoot, {
            "roomSettings": settings
        });
        dialog.show();
        destroyOnClose(dialog);
    }

    function showSpacePLApplyPrompt(settings, editingModel) {
        var dialog = Qt.createComponent("qrc:/qml/dialogs/PowerLevelSpacesApplyDialog.qml").createObject(timelineRoot, {
            "roomSettings": settings,
            "editingModel": editingModel
        });
        dialog.show();
        destroyOnClose(dialog);
    }

    function showAllowedRoomsEditor(settings) {
        var dialog = Qt.createComponent("qrc:/qml/dialogs/AllowedRoomsSettingsDialog.qml").createObject(timelineRoot, {
            "roomSettings": settings
        });
        dialog.show();
        destroyOnClose(dialog);
    }

    Component {
        id: readReceiptsDialog

        ReadReceipts {
        }

    }

    Shortcut {
        sequence: StandardKey.Quit
        onActivated: Qt.quit()
    }

    Shortcut {
        sequence: "Ctrl+K"
        onActivated: {
            var quickSwitch = Qt.createComponent("qrc:/qml/QuickSwitcher.qml").createObject(timelineRoot);
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
            var dialog = Qt.createComponent("qrc:/qml/dialogs/LogoutDialog.qml").createObject(timelineRoot);
            dialog.open();
            destroyOnClose(dialog);
        }

        function onOpenJoinRoomDialog() {
            var dialog = Qt.createComponent("qrc:/qml/dialogs/JoinRoomDialog.qml").createObject(timelineRoot);
            dialog.show();
            destroyOnClose(dialog);
        }

        function onShowRoomJoinPrompt(summary) {
            var dialog = Qt.createComponent("qrc:/qml/dialogs/ConfirmJoinRoomDialog.qml").createObject(timelineRoot, {"summary": summary});
            dialog.show();
            destroyOnClose(dialog);
        }

        target: Nheko
    }

    Connections {
        function onNewDeviceVerificationRequest(flow) {
            var dialog = Qt.createComponent("qrc:/qml/device-verification/DeviceVerification.qml").createObject(timelineRoot, {
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
            var userProfile = Qt.createComponent("qrc:/qml/dialogs/UserProfile.qml").createObject(timelineRoot, {
                "profile": profile
            });
            userProfile.show();
            destroyOnClose(userProfile);
        }

        function onShowImagePackSettings(room, packlist) {
            var packSet = Qt.createComponent("qrc:/qml/dialogs/ImagePackSettingsDialog.qml").createObject(timelineRoot, {
                "room": room,
                "packlist": packlist
            });
            packSet.show();
            destroyOnClose(packSet);
        }

        function onOpenRoomMembersDialog(members, room) {
            var membersDialog = Qt.createComponent("qrc:/qml/dialogs/RoomMembers.qml").createObject(timelineRoot, {
                "members": members,
                "room": room
            });
            membersDialog.show();
            destroyOnClose(membersDialog);
        }

        function onOpenRoomSettingsDialog(settings) {
            var roomSettings = Qt.createComponent("qrc:/qml/dialogs/RoomSettings.qml").createObject(timelineRoot, {
                "roomSettings": settings
            });
            roomSettings.show();
            destroyOnClose(roomSettings);
        }

        function onOpenInviteUsersDialog(invitees) {
            var component = Qt.createComponent("qrc:/qml/dialogs/InviteDialog.qml")
            var dialog = component.createObject(timelineRoot, {
                "roomId": Rooms.currentRoom.roomId,
                "plainRoomName": Rooms.currentRoom.plainRoomName,
                "invitees": invitees
            });
            if (component.status != Component.Ready) {
                console.log("Failed to create component: " + component.errorString());
            }
            dialog.show();
            destroyOnClose(dialog);
        }

        function onOpenLeaveRoomDialog(roomid, reason) {
            var dialog = Qt.createComponent("qrc:/qml/dialogs/LeaveRoomDialog.qml").createObject(timelineRoot, {
                "roomId": roomid,
                "reason": reason
            });
            dialog.open();
            destroyOnClose(dialog);
        }

        function onShowImageOverlay(room, eventId, url, originalWidth, proportionalHeight) {
            var dialog = Qt.createComponent("qrc:/qml/dialogs/ImageOverlay.qml").createObject(timelineRoot, {
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
                var dialog = Qt.createComponent("qrc:/qml/voip/CallInvite.qml").createObject(timelineRoot);
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
        prompt: qsTr("Please enter the token which has been sent to you:")
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

        Transition {
            id: reducedMotionTransitionExit
            PropertyAnimation {
                property: "opacity"
                from: 1
                to:0
                duration: 200
            }
        }
        Transition {
            id: reducedMotionTransitionEnter
            SequentialAnimation {
                PropertyAction { property: "opacity"; value: 0 }
                PauseAnimation { duration: 200 }
                PropertyAnimation {
                    property: "opacity"
                    from: 0
                    to:1
                    duration: 200
                }
            }
        }

        // for some reason direct bindings to a hidden StackView don't work, so manually store and restore here.
        property Transition pushEnterOrg
        property Transition pushExitOrg
        property Transition popEnterOrg
        property Transition popExitOrg
        property Transition replaceEnterOrg
        property Transition replaceExitOrg
        Component.onCompleted: {
            pushEnterOrg = pushEnter;
            popEnterOrg = popEnter;
            replaceEnterOrg = replaceEnter;
            pushExitOrg = pushExit;
            popExitOrg = popExit;
            replaceExitOrg = replaceExit;

            updateTrans()
        }

        function updateTrans() {
            pushEnter = Settings.reducedMotion ? reducedMotionTransitionEnter : pushEnterOrg;
            pushExit = Settings.reducedMotion ? reducedMotionTransitionExit : pushExitOrg;
            popEnter = Settings.reducedMotion ? reducedMotionTransitionEnter : popEnterOrg;
            popExit = Settings.reducedMotion ? reducedMotionTransitionExit : popExitOrg;
            replaceEnter = Settings.reducedMotion ? reducedMotionTransitionEnter : replaceEnterOrg;
            replaceExit = Settings.reducedMotion ? reducedMotionTransitionExit : replaceExitOrg;
        }

        Connections {
            target: Settings
            function onReducedMotionChanged() {
                mainWindow.updateTrans();
            }
        }
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

    Component {
        id: userSettingsPage

        UserSettingsPage {
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
