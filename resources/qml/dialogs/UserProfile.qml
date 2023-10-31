// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../ui"
import "../components"
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import QtQml.Models 2.2
import im.nheko 1.0

ApplicationWindow {
    id: userProfileDialog

    property var profile

    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 650
    minimumHeight: 150
    minimumWidth: 150
    modality: Qt.NonModal
    title: profile.isGlobalUserProfile ? qsTr("Global User Profile") : qsTr("Room User Profile")
    width: 420

    Shortcut {
        sequence: StandardKey.Cancel

        onActivated: userProfileDialog.close()
    }
    ListView {
        id: devicelist

        property int selectedTab: 0

        Layout.fillHeight: true
        Layout.fillWidth: true
        anchors.fill: parent
        anchors.margins: 10
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        footerPositioning: ListView.OverlayFooter
        model: (selectedTab == 0) ? devicesModel : sharedRoomsModel
        spacing: 8

        footer: DialogButtonBox {
            alignment: Qt.AlignRight
            standardButtons: DialogButtonBox.Ok
            width: devicelist.width
            z: 2

            background: Rectangle {
                anchors.fill: parent
                color: palette.window
            }

            onAccepted: userProfileDialog.close()
        }
        header: ColumnLayout {
            id: contentL

            spacing: Nheko.paddingMedium
            width: devicelist.width

            Avatar {
                id: displayAvatar

                Layout.alignment: Qt.AlignHCenter
                Layout.preferredHeight: 130
                Layout.preferredWidth: 130
                displayName: profile.displayName
                url: profile.avatarUrl.replace("mxc://", "image://MxcImage/")
                userid: profile.userid

                onClicked: TimelineManager.openImageOverlay(null, profile.avatarUrl, "", 0, 0)

                ImageButton {
                    ToolTip.text: profile.isGlobalUserProfile ? qsTr("Change avatar globally.") : qsTr("Change avatar. Will only apply to this room.")
                    ToolTip.visible: hovered
                    anchors.left: displayAvatar.left
                    anchors.leftMargin: Nheko.paddingMedium
                    anchors.top: displayAvatar.top
                    anchors.topMargin: Nheko.paddingMedium
                    hoverEnabled: true
                    image: ":/icons/icons/ui/edit.svg"
                    visible: profile.isSelf

                    onClicked: profile.changeAvatar()
                }
            }
            Spinner {
                Layout.alignment: Qt.AlignHCenter
                foreground: palette.mid
                running: profile.isLoading
                visible: profile.isLoading
            }
            Text {
                id: errorText

                Layout.alignment: Qt.AlignHCenter
                color: "red"
                opacity: 0
                visible: opacity > 0
            }
            SequentialAnimation {
                id: hideErrorAnimation

                running: false

                PauseAnimation {
                    duration: 4000
                }
                NumberAnimation {
                    duration: 1000
                    property: 'opacity'
                    target: errorText
                    to: 0
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

                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: parent.width - (Nheko.paddingSmall * 2) - usernameChangeButton.anchors.leftMargin - (usernameChangeButton.width * 2)
                color: TimelineManager.userColor(profile.userid, palette.window)
                font.bold: true
                font.pixelSize: 20
                horizontalAlignment: TextInput.AlignHCenter
                readOnly: !isUsernameEditingAllowed
                selectByMouse: true
                text: profile.displayName
                wrapMode: TextInput.Wrap

                onAccepted: {
                    profile.changeUsername(displayUsername.text);
                    displayUsername.isUsernameEditingAllowed = false;
                }

                ImageButton {
                    id: usernameChangeButton

                    ToolTip.text: profile.isGlobalUserProfile ? qsTr("Change display name globally.") : qsTr("Change display name. Will only apply to this room.")
                    ToolTip.visible: hovered
                    anchors.left: displayUsername.right
                    anchors.leftMargin: Nheko.paddingSmall
                    anchors.verticalCenter: displayUsername.verticalCenter
                    hoverEnabled: true
                    image: displayUsername.isUsernameEditingAllowed ? ":/icons/icons/ui/checkmark.svg" : ":/icons/icons/ui/edit.svg"
                    visible: profile.isSelf

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
                Layout.alignment: Qt.AlignHCenter
                text: profile.userid
            }
            MatrixText {
                id: statusMsg

                property string userStatus: Presence.userStatus(profile.userid)

                Layout.fillWidth: true
                Layout.leftMargin: Nheko.paddingMedium
                Layout.rightMargin: Nheko.paddingMedium
                font.pointSize: Math.floor(fontMetrics.font.pointSize * 0.9)
                horizontalAlignment: TextEdit.AlignHCenter
                text: qsTr("<i><b>Status:</b> %1</i>").arg(userStatus)
                visible: userStatus != ""

                Connections {
                    function onPresenceChanged(id) {
                        if (id == profile.userid)
                            statusMsg.userStatus = Presence.userStatus(profile.userid);
                    }

                    target: Presence
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Nheko.paddingSmall
                visible: !profile.isGlobalUserProfile

                MatrixText {
                    id: displayRoomname

                    Layout.maximumWidth: parent.parent.width - (parent.spacing * 3) - 16
                    ToolTip.text: qsTr("This is a room-specific profile. The user's name and avatar may be different from their global versions.")
                    ToolTip.visible: ma.hovered
                    horizontalAlignment: TextEdit.AlignHCenter
                    text: qsTr("Room: %1").arg(profile.room ? profile.room.roomName : "")

                    HoverHandler {
                        id: ma

                    }
                }
                ImageButton {
                    ToolTip.text: qsTr("Open the global profile for this user.")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/world.svg"

                    onClicked: profile.openGlobalProfile()
                }
            }
            Button {
                id: verifyUserButton

                Layout.alignment: Qt.AlignHCenter
                enabled: profile.userVerified != Crypto.Verified
                text: qsTr("Verify")
                visible: profile.userVerified != Crypto.Verified && !profile.isSelf && profile.userVerificationEnabled

                onClicked: profile.verify()
            }
            EncryptionIndicator {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredHeight: 32
                Layout.preferredWidth: 32
                ToolTip.visible: false
                encrypted: profile.userVerificationEnabled
                trust: profile.userVerified
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
                    Layout.preferredHeight: 24
                    Layout.preferredWidth: 24
                    ToolTip.text: qsTr("Start a private chat.")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/chat.svg"

                    onClicked: profile.startChat()
                }
                ImageButton {
                    Layout.preferredHeight: 24
                    Layout.preferredWidth: 24
                    ToolTip.text: qsTr("Kick the user.")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/round-remove-button.svg"
                    visible: !profile.isGlobalUserProfile && profile.room.permissions.canKick()

                    onClicked: profile.kickUser()
                }
                ImageButton {
                    Layout.preferredHeight: 24
                    Layout.preferredWidth: 24
                    ToolTip.text: qsTr("Ban the user.")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/ban.svg"
                    visible: !profile.isGlobalUserProfile && profile.room.permissions.canBan()

                    onClicked: profile.banUser()
                }
                ImageButton {
                    Layout.preferredHeight: 24
                    Layout.preferredWidth: 24
                    ToolTip.text: profile.ignored ? qsTr("Unignore the user.") : qsTr("Ignore the user.")
                    ToolTip.visible: hovered
                    buttonTextColor: profile.ignored ? Nheko.theme.red : palette.buttonText
                    hoverEnabled: true
                    image: ":/icons/icons/ui/volume-off-indicator.svg"
                    visible: !profile.isSelf

                    onClicked: profile.ignored = !profile.ignored
                }
                ImageButton {
                    Layout.preferredHeight: 24
                    Layout.preferredWidth: 24
                    ToolTip.text: qsTr("Refresh device list.")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/refresh.svg"

                    onClicked: profile.refreshDevices()
                }
            }
            TabBar {
                id: tabbar

                Layout.bottomMargin: Nheko.paddingMedium
                Layout.fillWidth: true
                visible: !profile.isSelf

                onCurrentIndexChanged: devicelist.selectedTab = currentIndex

                NhekoTabButton {
                    text: qsTr("Devices")
                }
                NhekoTabButton {
                    text: qsTr("Shared Rooms")
                }
            }
        }

        DelegateModel {
            id: devicesModel

            model: profile.deviceList

            delegate: RowLayout {
                required property string deviceId
                required property string deviceName
                required property string lastIp
                required property var lastTs
                required property int verificationStatus

                spacing: 4
                width: devicelist.width

                ColumnLayout {
                    Layout.leftMargin: Nheko.paddingMedium
                    Layout.rightMargin: Nheko.paddingMedium
                    spacing: 0

                    RowLayout {
                        Text {
                            Layout.alignment: Qt.AlignLeft
                            Layout.fillWidth: true
                            color: palette.text
                            elide: Text.ElideRight
                            font.bold: true
                            text: deviceId
                        }
                        Image {
                            Layout.preferredHeight: 16
                            Layout.preferredWidth: 16
                            source: {
                                switch (verificationStatus) {
                                case VerificationStatus.VERIFIED:
                                    return "image://colorimage/:/icons/icons/ui/shield-filled-checkmark.svg?" + Nheko.theme.green;
                                case VerificationStatus.UNVERIFIED:
                                    return "image://colorimage/:/icons/icons/ui/shield-filled-exclamation-mark.svg?" + Nheko.theme.orange;
                                case VerificationStatus.SELF:
                                    return "image://colorimage/:/icons/icons/ui/checkmark.svg?" + Nheko.theme.green;
                                default:
                                    return "image://colorimage/:/icons/icons/ui/shield-filled-cross.svg?" + Nheko.theme.orange;
                                }
                            }
                            sourceSize.height: 16 * Screen.devicePixelRatio
                            sourceSize.width: 16 * Screen.devicePixelRatio
                            visible: profile.isSelf && verificationStatus != VerificationStatus.NOT_APPLICABLE
                        }
                        ImageButton {
                            Layout.alignment: Qt.AlignTop
                            ToolTip.text: qsTr("Sign out this device.")
                            ToolTip.visible: hovered
                            hoverEnabled: true
                            image: ":/icons/icons/ui/power-off.svg"
                            visible: profile.isSelf

                            onClicked: profile.signOutDevice(deviceId)
                        }
                    }
                    RowLayout {
                        id: deviceNameRow

                        property bool isEditingAllowed

                        TextInput {
                            id: deviceNameField

                            Layout.alignment: Qt.AlignLeft
                            Layout.fillWidth: true
                            color: palette.text
                            readOnly: !deviceNameRow.isEditingAllowed
                            selectByMouse: true
                            text: deviceName

                            onAccepted: {
                                profile.changeDeviceName(deviceId, deviceNameField.text);
                                deviceNameRow.isEditingAllowed = false;
                            }
                        }
                        ImageButton {
                            ToolTip.text: qsTr("Change device name.")
                            ToolTip.visible: hovered
                            hoverEnabled: true
                            image: deviceNameRow.isEditingAllowed ? ":/icons/icons/ui/checkmark.svg" : ":/icons/icons/ui/edit.svg"
                            visible: profile.isSelf

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
                        Layout.alignment: Qt.AlignLeft
                        Layout.fillWidth: true
                        color: palette.text
                        elide: Text.ElideRight
                        text: qsTr("Last seen %1 from %2").arg(new Date(lastTs).toLocaleString(Locale.ShortFormat)).arg(lastIp ? lastIp : "???")
                        visible: profile.isSelf
                    }
                }
                Image {
                    Layout.preferredHeight: 16
                    Layout.preferredWidth: 16
                    source: {
                        switch (verificationStatus) {
                        case VerificationStatus.VERIFIED:
                            return "image://colorimage/:/icons/icons/ui/shield-filled-checkmark.svg?" + Nheko.theme.green;
                        case VerificationStatus.UNVERIFIED:
                            return "image://colorimage/:/icons/icons/ui/shield-filled-exclamation-mark.svg?" + Nheko.theme.orange;
                        case VerificationStatus.SELF:
                            return "image://colorimage/:/icons/icons/ui/checkmark.svg?" + Nheko.theme.green;
                        default:
                            return "image://colorimage/:/icons/icons/ui/shield-filled.svg?" + Nheko.theme.red;
                        }
                    }
                    visible: !profile.isSelf && verificationStatus != VerificationStatus.NOT_APPLICABLE
                }
                Button {
                    id: verifyButton

                    text: (verificationStatus != VerificationStatus.VERIFIED) ? qsTr("Verify") : qsTr("Unverify")
                    visible: verificationStatus == VerificationStatus.UNVERIFIED && (profile.isSelf || !profile.userVerificationEnabled)

                    onClicked: {
                        if (verificationStatus == VerificationStatus.VERIFIED)
                            profile.unverify(deviceId);
                        else
                            profile.verify(deviceId);
                    }
                }
            }
        }
        DelegateModel {
            id: sharedRoomsModel

            model: profile.sharedRooms

            delegate: RowLayout {
                required property string avatarUrl
                required property string roomId
                required property string roomName

                spacing: 4
                width: devicelist.width

                Avatar {
                    id: avatar

                    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 1.6)

                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: Nheko.paddingMedium
                    Layout.preferredHeight: avatarSize
                    Layout.preferredWidth: avatarSize
                    displayName: roomName
                    enabled: false
                    roomid: roomId
                    url: avatarUrl.replace("mxc://", "image://MxcImage/")
                }
                ElidedLabel {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                    Layout.rightMargin: Nheko.paddingMedium
                    color: palette.text
                    elideWidth: width
                    fullText: roomName
                    textFormat: Text.PlainText
                }
                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }
}
