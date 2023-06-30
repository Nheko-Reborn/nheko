// SPDX-FileCopyrightText: Nheko Contributors
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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import im.nheko

Pane {
    id: timelineRoot

    function destroyOnClose(obj) {
        if (obj.closing != undefined)
            obj.closing.connect(() => obj.destroy(1000));
        else if (obj.aboutToHide != undefined)
            obj.aboutToHide.connect(() => obj.destroy(1000));
    }
    function destroyOnClosed(obj) {
        obj.aboutToHide.connect(() => obj.destroy(1000));
    }

    //Timer {
    //    onTriggered: gc()
    //    interval: 1000
    //    running: true
    //    repeat: true
    //}
    function showAliasEditor(settings) {
        var component = Qt.createComponent("qrc:/resources/qml/dialogs/AliasEditor.qml");
        if (component.status == Component.Ready) {
            var dialog = component.createObject(timelineRoot, {
                    "roomSettings": settings
                });
            dialog.show();
            destroyOnClose(dialog);
        } else {
            console.error("Failed to create component: " + component.errorString());
        }
    }
    function showAllowedRoomsEditor(settings) {
        var component = Qt.createComponent("qrc:/resources/qml/dialogs/AllowedRoomsSettingsDialog.qml");
        if (component.status == Component.Ready) {
            var dialog = component.createObject(timelineRoot, {
                    "roomSettings": settings
                });
            dialog.show();
            destroyOnClose(dialog);
        } else {
            console.error("Failed to create component: " + component.errorString());
        }
    }
    function showPLEditor(settings) {
        var component = Qt.createComponent("qrc:/resources/qml/dialogs/PowerLevelEditor.qml");
        if (component.status == Component.Ready) {
            var dialog = component.createObject(timelineRoot, {
                    "roomSettings": settings
                });
            dialog.show();
            destroyOnClose(dialog);
        } else {
            console.error("Failed to create component: " + component.errorString());
        }
    }
    function showSpacePLApplyPrompt(settings, editingModel) {
        var component = Qt.createComponent("qrc:/resources/qml/dialogs/PowerLevelSpacesApplyDialog.qml");
        if (component.status == Component.Ready) {
            var dialog = component.createObject(timelineRoot, {
                    "roomSettings": settings,
                    "editingModel": editingModel
                });
            dialog.show();
            destroyOnClose(dialog);
        } else {
            console.error("Failed to create component: " + component.errorString());
        }
    }

    background: null
    padding: 0

    FontMetrics {
        id: fontMetrics

    }
    UserDirectoryModel {
        id: userDirectory

    }
    RoomDirectoryModel {
        id: publicRooms

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
            var component = Qt.createComponent("qrc:/resources/qml/QuickSwitcher.qml");
            if (component.status == Component.Ready) {
                var quickSwitch = component.createObject(timelineRoot);
                quickSwitch.open();
                destroyOnClosed(quickSwitch);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
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
        function onOpenJoinRoomDialog() {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/JoinRoomDialog.qml");
            if (component.status == Component.Ready) {
                var dialog = component.createObject(timelineRoot);
                dialog.show();
                destroyOnClose(dialog);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }
        function onOpenLogoutDialog() {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/LogoutDialog.qml");
            if (component.status == Component.Ready) {
                var dialog = component.createObject(timelineRoot);
                dialog.open();
                destroyOnClose(dialog);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }
        function onShowRoomJoinPrompt(summary) {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/ConfirmJoinRoomDialog.qml");
            if (component.status == Component.Ready) {
                var dialog = component.createObject(timelineRoot, {
                        "summary": summary
                    });
                dialog.show();
                destroyOnClose(dialog);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }

        target: Nheko
    }
    Connections {
        function onNewDeviceVerificationRequest(flow) {
            var component = Qt.createComponent("qrc:/resources/qml/device-verification/DeviceVerification.qml");
            if (component.status == Component.Ready) {
                var dialog = component.createObject(timelineRoot, {
                        "flow": flow
                    });
                dialog.show();
                destroyOnClose(dialog);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }

        target: VerificationManager
    }
    Connections {
        function onOpenInviteUsersDialog(invitees) {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/InviteDialog.qml");
            if (component.status == Component.Ready) {
                var dialog = component.createObject(timelineRoot, {
                        "invitees": invitees
                    });
                dialog.show();
                destroyOnClose(dialog);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }
        function onOpenLeaveRoomDialog(roomid, reason) {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/LeaveRoomDialog.qml");
            if (component.status == Component.Ready) {
                var dialog = component.createObject(timelineRoot, {
                        "roomId": roomid,
                        "reason": reason
                    });
                dialog.open();
                destroyOnClose(dialog);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }
        function onOpenProfile(profile) {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/UserProfile.qml");
            if (component.status == Component.Ready) {
                var userProfile = component.createObject(timelineRoot, {
                        "profile": profile
                    });
                userProfile.show();
                destroyOnClose(userProfile);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }
        function onOpenRoomMembersDialog(members, room) {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/RoomMembers.qml");
            if (component.status == Component.Ready) {
                var membersDialog = component.createObject(timelineRoot, {
                        "members": members,
                        "room": room
                    });
                membersDialog.show();
                destroyOnClose(membersDialog);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }
        function onOpenRoomSettingsDialog(settings) {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/RoomSettings.qml");
            if (component.status == Component.Ready) {
                var roomSettings = component.createObject(timelineRoot, {
                        "roomSettings": settings
                    });
                roomSettings.show();
                destroyOnClose(roomSettings);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }
        function onShowImageOverlay(room, eventId, url, originalWidth, proportionalHeight) {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/ImageOverlay.qml");
            if (component.status == Component.Ready) {
                var dialog = component.createObject(timelineRoot, {
                        "room": room,
                        "eventId": eventId,
                        "url": url,
                        "originalWidth": originalWidth ?? 0,
                        "proportionalHeight": proportionalHeight ?? 0
                    });
                dialog.showFullScreen();
                destroyOnClose(dialog);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }
        function onShowImagePackSettings(room, packlist) {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/ImagePackSettingsDialog.qml");
            if (component.status == Component.Ready) {
                var packSet = component.createObject(timelineRoot, {
                        "room": room,
                        "packlist": packlist
                    });
                packSet.show();
                destroyOnClose(packSet);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }

        target: TimelineManager
    }
    Connections {
        function onNewInviteState() {
            if (CallManager.haveCallInvite && Settings.mobileMode) {
                var component = Qt.createComponent("qrc:/resources/qml/voip/CallInvite.qml");
                if (component.status == Component.Ready) {
                    var dialog = component.createObject(timelineRoot);
                    dialog.open();
                    destroyOnClose(dialog);
                } else {
                    console.error("Failed to create component: " + component.errorString());
                }
            }
        }

        target: CallManager
    }
    SelfVerificationCheck {
    }
    InputDialog {
        id: uiaPassPrompt

        echoMode: TextInput.Password
        prompt: qsTr("Please enter your login password to continue:")
        title: UIA.title

        onAccepted: t => {
            return UIA.continuePassword(t);
        }
    }
    InputDialog {
        id: uiaEmailPrompt

        prompt: qsTr("Please enter a valid email address to continue:")
        title: UIA.title

        onAccepted: t => {
            return UIA.continueEmail(t);
        }
    }
    PhoneNumberInputDialog {
        id: uiaPhoneNumberPrompt

        prompt: qsTr("Please enter a valid phone number to continue:")
        title: UIA.title

        onAccepted: (p, t) => {
            return UIA.continuePhoneNumber(p, t);
        }
    }
    InputDialog {
        id: uiaTokenPrompt

        prompt: qsTr("Please enter the token which has been sent to you:")
        title: UIA.title

        onAccepted: t => {
            return UIA.submit3pidToken(t);
        }
    }
    Platform.MessageDialog {
        id: uiaConfirmationLinkDialog

        buttons: Platform.MessageDialog.Ok
        text: qsTr("Wait for the confirmation link to arrive, then continue.")

        onAccepted: UIA.continue3pidReceived()
    }

    Connections {
        function onConfirm3pidToken() {
            uiaConfirmationLinkDialog.open();
        }
        function onEmail() {
            uiaEmailPrompt.show();
        }
        function onPassword() {
            console.log("UIA: password needed");
            uiaPassPrompt.show();
        }
        function onPhoneNumber() {
            uiaPhoneNumberPrompt.show();
        }
        function onPrompt3pidToken() {
            uiaTokenPrompt.show();
        }
        function onReCaptcha(recaptcha) {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/ReCaptchaDialog.qml");
            if (component.status == Component.Ready) {
                var dialog = component.createObject(timelineRoot, {
                        "recaptcha": recaptcha
                    });
                dialog.show();
                destroyOnClose(dialog);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }

        target: UIA
    }
    StackView {
        id: mainWindow

        property Transition popEnterOrg
        property Transition popExitOrg

        // for some reason direct bindings to a hidden StackView don't work, so manually store and restore here.
        property Transition pushEnterOrg
        property Transition pushExitOrg
        property Transition replaceEnterOrg
        property Transition replaceExitOrg

        function updateTrans() {
            pushEnter = Settings.reducedMotion ? reducedMotionTransitionEnter : pushEnterOrg;
            pushExit = Settings.reducedMotion ? reducedMotionTransitionExit : pushExitOrg;
            popEnter = Settings.reducedMotion ? reducedMotionTransitionEnter : popEnterOrg;
            popExit = Settings.reducedMotion ? reducedMotionTransitionExit : popExitOrg;
            replaceEnter = Settings.reducedMotion ? reducedMotionTransitionEnter : replaceEnterOrg;
            replaceExit = Settings.reducedMotion ? reducedMotionTransitionExit : replaceExitOrg;
        }

        anchors.fill: parent
        initialItem: welcomePage

        Component.onCompleted: {
            pushEnterOrg = pushEnter;
            popEnterOrg = popEnter;
            replaceEnterOrg = replaceEnter;
            pushExitOrg = pushExit;
            popExitOrg = popExit;
            replaceExitOrg = replaceExit;
            updateTrans();
        }

        Transition {
            id: reducedMotionTransitionExit

            PropertyAnimation {
                duration: 200
                from: 1
                property: "opacity"
                to: 0
            }
        }
        Transition {
            id: reducedMotionTransitionEnter

            SequentialAnimation {
                PropertyAction {
                    property: "opacity"
                    value: 0
                }
                PauseAnimation {
                    duration: 200
                }
                PropertyAnimation {
                    duration: 200
                    from: 0
                    property: "opacity"
                    to: 1
                }
            }
        }
        Connections {
            function onReducedMotionChanged() {
                mainWindow.updateTrans();
            }

            target: Settings
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
    Snackbar {
        id: snackbar

    }
    Connections {
        function onShowNotification(msg) {
            snackbar.showNotification(msg);
            console.log("New snack: " + msg);
        }
        function onSwitchToChatPage() {
            mainWindow.replace(null, chatPage);
        }
        function onSwitchToLoginPage(error) {
            mainWindow.replace(welcomePage, {}, loginPage, {
                    "error": error
                }, StackView.PopTransition);
        }

        target: MainWindow
    }
}
