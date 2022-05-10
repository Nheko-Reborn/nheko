// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../device-verification"
import "../ui"
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0

ApplicationWindow {
    id: userProfileDialog

    property var profile

    height: 650
    width: 420
    minimumWidth: 150
    minimumHeight: 150
    palette: Nheko.colors
    color: Nheko.colors.window
    title: profile.isGlobalUserProfile ? qsTr("Global User Profile") : qsTr("Room User Profile")
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: userProfileDialog.close()
    }

    ListView {
        id: devicelist

        Layout.fillHeight: true
        Layout.fillWidth: true
        clip: true
        spacing: 8
        boundsBehavior: Flickable.StopAtBounds
        model: profile.deviceList
        anchors.fill: parent
        anchors.margins: 10
        footerPositioning: ListView.OverlayFooter

        ScrollHelper {
            flickable: parent
            anchors.fill: parent
            enabled: !Settings.mobileMode
        }

        header: ColumnLayout {
            id: contentL

            width: devicelist.width
            spacing: 10

            Avatar {
                id: displayAvatar

                url: profile.avatarUrl.replace("mxc://", "image://MxcImage/")
                height: 130
                width: 130
                displayName: profile.displayName
                userid: profile.userid
                Layout.alignment: Qt.AlignHCenter
                onClicked: TimelineManager.openImageOverlay(null, profile.avatarUrl, "", 0, 0)

                ImageButton {
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: profile.isGlobalUserProfile ? qsTr("Change avatar globally.") : qsTr("Change avatar. Will only apply to this room.")
                    anchors.left: displayAvatar.left
                    anchors.top: displayAvatar.top
                    anchors.leftMargin: Nheko.paddingMedium
                    anchors.topMargin: Nheko.paddingMedium
                    visible: profile.isSelf
                    image: ":/icons/icons/ui/edit.svg"
                    onClicked: profile.changeAvatar()
                }

            }

            Spinner {
                Layout.alignment: Qt.AlignHCenter
                running: profile.isLoading
                visible: profile.isLoading
                foreground: Nheko.colors.mid
            }

            Text {
                id: errorText

                color: "red"
                visible: opacity > 0
                opacity: 0
                Layout.alignment: Qt.AlignHCenter
            }

            SequentialAnimation {
                id: hideErrorAnimation

                running: false

                PauseAnimation {
                    duration: 4000
                }

                NumberAnimation {
                    target: errorText
                    property: 'opacity'
                    to: 0
                    duration: 1000
                }

            }

            Connections {
                function onDisplayError(errorMessage) {
                    errorText.text = errorMessage;
                    errorText.opacity = 1;
                    hideErrorAnimation.restart();
                }

                target: profile
            }

            TextInput {
                id: displayUsername

                property bool isUsernameEditingAllowed

                readOnly: !isUsernameEditingAllowed
                text: profile.displayName
                font.pixelSize: 20
                color: TimelineManager.userColor(profile.userid, Nheko.colors.window)
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: parent.width - (Nheko.paddingSmall * 2) - usernameChangeButton.anchors.leftMargin - (usernameChangeButton.width * 2)
                horizontalAlignment: TextInput.AlignHCenter
                wrapMode: TextInput.Wrap
                selectByMouse: true
                onAccepted: {
                    profile.changeUsername(displayUsername.text);
                    displayUsername.isUsernameEditingAllowed = false;
                }

                ImageButton {
                    id: usernameChangeButton
                    visible: profile.isSelf
                    anchors.leftMargin: Nheko.paddingSmall
                    anchors.left: displayUsername.right
                    anchors.verticalCenter: displayUsername.verticalCenter
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: profile.isGlobalUserProfile ? qsTr("Change display name globally.") : qsTr("Change display name. Will only apply to this room.")
                    image: displayUsername.isUsernameEditingAllowed ? ":/icons/icons/ui/checkmark.svg" : ":/icons/icons/ui/edit.svg"
                    onClicked: {
                        if (displayUsername.isUsernameEditingAllowed) {
                            profile.changeUsername(displayUsername.text);
                            displayUsername.isUsernameEditingAllowed = false;
                        } else {
                            displayUsername.isUsernameEditingAllowed = true;
                            displayUsername.focus = true;
                            displayUsername.selectAll();
                        }
                    }
                }

            }

            MatrixText {
                text: profile.userid
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                visible: !profile.isGlobalUserProfile
                Layout.alignment: Qt.AlignHCenter
                spacing: Nheko.paddingSmall

                MatrixText {
                    id: displayRoomname

                    text: qsTr("Room: %1").arg(profile.room ? profile.room.roomName : "")
                    ToolTip.text: qsTr("This is a room-specific profile. The user's name and avatar may be different from their global versions.")
                    ToolTip.visible: ma.hovered
                    Layout.maximumWidth: parent.parent.width - (parent.spacing * 3) - 16
                    horizontalAlignment: TextEdit.AlignHCenter

                    HoverHandler {
                        id: ma
                    }

                }

                ImageButton {
                    image: ":/icons/icons/ui/world.svg"
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Open the global profile for this user.")
                    onClicked: profile.openGlobalProfile()
                }

            }

            Button {
                id: verifyUserButton

                text: qsTr("Verify")
                Layout.alignment: Qt.AlignHCenter
                enabled: profile.userVerified != Crypto.Verified
                visible: profile.userVerified != Crypto.Verified && !profile.isSelf && profile.userVerificationEnabled
                onClicked: profile.verify()
            }

            EncryptionIndicator {
                Layout.preferredHeight: 16
                Layout.preferredWidth: 16
                encrypted: profile.userVerificationEnabled
                trust: profile.userVerified
                Layout.alignment: Qt.AlignHCenter
                ToolTip.visible: false
            }

            RowLayout {
                // ImageButton{
                //     image:":/icons/icons/ui/volume-off-indicator.svg"
                //     Layout.margins: {
                //         left: 5
                //         right: 5
                //     }
                //     ToolTip.visible: hovered
                //     ToolTip.text: qsTr("Ignore messages from this user.")
                //     onClicked : {
                //         profile.ignoreUser()
                //     }
                // }

                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 10
                spacing: Nheko.paddingSmall

                ImageButton {
                    image: ":/icons/icons/ui/chat.svg"
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Start a private chat.")
                    onClicked: profile.startChat()
                }

                ImageButton {
                    image: ":/icons/icons/ui/round-remove-button.svg"
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Kick the user.")
                    onClicked: profile.kickUser()
                    visible: !profile.isGlobalUserProfile && profile.room.permissions.canKick()
                }

                ImageButton {
                    image: ":/icons/icons/ui/ban.svg"
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Ban the user.")
                    onClicked: profile.banUser()
                    visible: !profile.isGlobalUserProfile && profile.room.permissions.canBan()
                }

                ImageButton {
                    image: ":/icons/icons/ui/refresh.svg"
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Refresh device list.")
                    onClicked: profile.refreshDevices()
                }

            }

        }

        delegate: RowLayout {
            required property int verificationStatus
            required property string deviceId
            required property string deviceName
            required property string lastIp
            required property var lastTs

            width: devicelist.width
            spacing: 4

            ColumnLayout {
                spacing: 0

                RowLayout {
                    Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft
                        elide: Text.ElideRight
                        font.bold: true
                        color: Nheko.colors.text
                        text: deviceId
                    }

                    Image {
                        Layout.preferredHeight: 16
                        Layout.preferredWidth: 16
                        visible: profile.isSelf && verificationStatus != VerificationStatus.NOT_APPLICABLE
                        sourceSize.height: 16 * Screen.devicePixelRatio
                        sourceSize.width: 16 * Screen.devicePixelRatio
                        source: {
                            switch (verificationStatus) {
                            case VerificationStatus.VERIFIED:
                                return "image://colorimage/:/icons/icons/ui/shield-filled-checkmark.svg?green";
                            case VerificationStatus.UNVERIFIED:
                                return "image://colorimage/:/icons/icons/ui/shield-filled-exclamation-mark.svg?#d6c020";
                            case VerificationStatus.SELF:
                                return "image://colorimage/:/icons/icons/ui/checkmark.svg?green";
                            default:
                                return "image://colorimage/:/icons/icons/ui/shield-filled-cross.svg?#d6c020";
                            }
                        }
                    }

                    ImageButton {
                        Layout.alignment: Qt.AlignTop
                        image: ":/icons/icons/ui/power-off.svg"
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Sign out this device.")
                        onClicked: profile.signOutDevice(deviceId)
                        visible: profile.isSelf
                    }

                }

                RowLayout {
                    id: deviceNameRow

                    property bool isEditingAllowed

                    TextInput {
                        id: deviceNameField

                        readOnly: !deviceNameRow.isEditingAllowed
                        text: deviceName
                        color: Nheko.colors.text
                        Layout.alignment: Qt.AlignLeft
                        Layout.fillWidth: true
                        selectByMouse: true
                        onAccepted: {
                            profile.changeDeviceName(deviceId, deviceNameField.text);
                            deviceNameRow.isEditingAllowed = false;
                        }
                    }

                    ImageButton {
                        visible: profile.isSelf
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Change device name.")
                        image: deviceNameRow.isEditingAllowed ? ":/icons/icons/ui/checkmark.svg" : ":/icons/icons/ui/edit.svg"
                        onClicked: {
                            if (deviceNameRow.isEditingAllowed) {
                                profile.changeDeviceName(deviceId, deviceNameField.text);
                                deviceNameRow.isEditingAllowed = false;
                            } else {
                                deviceNameRow.isEditingAllowed = true;
                                deviceNameField.focus = true;
                                deviceNameField.selectAll();
                            }
                        }
                    }

                }

                Text {
                    visible: profile.isSelf
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignLeft
                    elide: Text.ElideRight
                    color: Nheko.colors.text
                    text: qsTr("Last seen %1 from %2").arg(new Date(lastTs).toLocaleString(Locale.ShortFormat)).arg(lastIp ? lastIp : "???")
                }

            }

            Image {
                Layout.preferredHeight: 16
                Layout.preferredWidth: 16
                visible: !profile.isSelf && verificationStatus != VerificationStatus.NOT_APPLICABLE
                source: {
                    switch (verificationStatus) {
                    case VerificationStatus.VERIFIED:
                        return "image://colorimage/:/icons/icons/ui/shield-filled-checkmark.svg?green";
                    case VerificationStatus.UNVERIFIED:
                        return "image://colorimage/:/icons/icons/ui/shield-filled-exclamation-mark.svg?#d6c020";
                    case VerificationStatus.SELF:
                        return "image://colorimage/:/icons/icons/ui/checkmark.svg?green";
                    default:
                        return "image://colorimage/:/icons/icons/ui/shield-filled.svg?red";
                    }
                }
            }

            Button {
                id: verifyButton

                visible: verificationStatus == VerificationStatus.UNVERIFIED && (profile.isSelf || !profile.userVerificationEnabled)
                text: (verificationStatus != VerificationStatus.VERIFIED) ? qsTr("Verify") : qsTr("Unverify")
                onClicked: {
                    if (verificationStatus == VerificationStatus.VERIFIED)
                        profile.unverify(deviceId);
                    else
                        profile.verify(deviceId);
                }
            }

        }

        footer: DialogButtonBox {
            z: 2
            width: devicelist.width
            alignment: Qt.AlignRight
            standardButtons: DialogButtonBox.Ok
            onAccepted: userProfileDialog.close()

            background: Rectangle {
                anchors.fill: parent
                color: Nheko.colors.window
            }

        }

    }

}
