// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./components/"
import Qt.labs.platform 1.1 as P
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import im.nheko 1.0

Item {
    visible: false
    enabled: false

    Dialog {
        id: showRecoverKeyDialog

        property string recoveryKey: ""

        parent: Overlay.overlay
        anchors.centerIn: parent
        height: content.height + implicitFooterHeight + implicitHeaderHeight
        width: content.width
        padding: 0
        modal: true
        standardButtons: Dialog.Ok
        closePolicy: Popup.NoAutoClose

        ColumnLayout {
            id: content

            spacing: 0

            Label {
                Layout.margins: Nheko.paddingMedium
                Layout.maximumWidth: (Overlay.overlay ? Overlay.overlay.width : 400) - Nheko.paddingMedium * 4
                Layout.fillWidth: true
                text: qsTr("This is your recovery key. You will need it to restore access to your encrypted messages and verification keys. Keep this safe. Don't share it with anyone and don't lose it! Do not pass go! Do not collect $200!")
                color: Nheko.colors.text
                wrapMode: Text.Wrap
            }

            TextEdit {
                Layout.maximumWidth: (Overlay.overlay ? Overlay.overlay.width : 400) - Nheko.paddingMedium * 4
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: TextEdit.AlignHCenter
                verticalAlignment: TextEdit.AlignVCenter
                readOnly: true
                selectByMouse: true
                text: showRecoverKeyDialog.recoveryKey
                color: Nheko.colors.text
                font.bold: true
                wrapMode: TextEdit.Wrap
            }

        }

        background: Rectangle {
            color: Nheko.colors.window
            border.color: Nheko.theme.separator
            border.width: 1
            radius: Nheko.paddingSmall
        }

    }

    P.MessageDialog {
        id: successDialog

        buttons: P.MessageDialog.Ok
        text: qsTr("Encryption setup successfully")
    }

    P.MessageDialog {
        id: failureDialog

        property string errorMessage

        buttons: P.MessageDialog.Ok
        text: qsTr("Failed to setup encryption: %1").arg(errorMessage)
    }

    MainWindowDialog {
        id: bootstrapCrosssigning

        onAccepted: SelfVerificationStatus.setupCrosssigning(storeSecretsOnline.checked, usePassword.checked ? passwordField.text : "", useOnlineKeyBackup.checked)

        GridLayout {
            id: grid

            width: bootstrapCrosssigning.useableWidth
            columns: 2
            rowSpacing: 0
            columnSpacing: 0
            z: 1

            Label {
                Layout.margins: Nheko.paddingMedium
                Layout.alignment: Qt.AlignHCenter
                Layout.columnSpan: 2
                font.pointSize: fontMetrics.font.pointSize * 2
                text: qsTr("Setup Encryption")
                color: Nheko.colors.text
                wrapMode: Text.Wrap
            }

            Label {
                Layout.margins: Nheko.paddingMedium
                Layout.alignment: Qt.AlignLeft
                Layout.columnSpan: 2
                Layout.maximumWidth: grid.width - Nheko.paddingMedium * 2
                text: qsTr("Hello and welcome to Matrix!\nIt seems like you are new. Before you can securely encrypt your messages, we need to setup a few small things. You can either press accept immediately or adjust a few basic options. We also try to explain a few of the basics. You can skip those parts, but they might prove to be helpful!")
                color: Nheko.colors.text
                wrapMode: Text.Wrap
            }

            Label {
                Layout.margins: Nheko.paddingMedium
                Layout.alignment: Qt.AlignLeft
                Layout.columnSpan: 1
                Layout.maximumWidth: Math.floor(grid.width / 2) - Nheko.paddingMedium * 2
                text: "Store secrets online.\nYou have a few secrets to make all the encryption magic work. While you can keep them stored only locally, we recommend storing them encrypted on the server. Otherwise it will be painful to recover them. Only disable this if you are paranoid and like losing your data!"
                color: Nheko.colors.text
                wrapMode: Text.Wrap
            }

            Item {
                Layout.margins: Nheko.paddingMedium
                Layout.preferredHeight: storeSecretsOnline.height
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.fillWidth: true

                ToggleButton {
                    id: storeSecretsOnline

                    checked: true
                    onClicked: console.log("Store secrets toggled: " + checked)
                }

            }

            Label {
                Layout.margins: Nheko.paddingMedium
                Layout.alignment: Qt.AlignLeft
                Layout.columnSpan: 1
                Layout.rowSpan: 2
                Layout.maximumWidth: Math.floor(grid.width / 2) - Nheko.paddingMedium * 2
                visible: storeSecretsOnline.checked
                text: "Set an online backup password.\nWe recommend you DON'T set a password and instead only rely on the recovery key. You will get a recovery key in any case when storing the cross-signing secrets online, but passwords are usually not very random, so they are easier to attack than a completely random recovery key. If you choose to use a password, DON'T make it the same as your login password, otherwise your server can read all your encrypted messages. (You don't want that.)"
                color: Nheko.colors.text
                wrapMode: Text.Wrap
            }

            Item {
                Layout.margins: Nheko.paddingMedium
                Layout.topMargin: Nheko.paddingLarge
                Layout.preferredHeight: storeSecretsOnline.height
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.rowSpan: usePassword.checked ? 1 : 2
                Layout.fillWidth: true
                visible: storeSecretsOnline.checked

                ToggleButton {
                    id: usePassword

                    checked: false
                }

            }

            MatrixTextField {
                id: passwordField

                Layout.margins: Nheko.paddingMedium
                Layout.maximumWidth: Math.floor(grid.width / 2) - Nheko.paddingMedium * 2
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.columnSpan: 1
                Layout.fillWidth: true
                visible: storeSecretsOnline.checked && usePassword.checked
                echoMode: TextInput.Password
            }

            Label {
                Layout.margins: Nheko.paddingMedium
                Layout.alignment: Qt.AlignLeft
                Layout.columnSpan: 1
                Layout.maximumWidth: Math.floor(grid.width / 2) - Nheko.paddingMedium * 2
                text: "Use online key backup.\nStore the keys for your messages securely encrypted online. In general you do want this, because it protects your messages from becoming unreadable, if you log out by accident. It does however carry a small security risk, if you ever share your recovery key by accident. Currently this also has some other weaknesses, that might allow the server to insert new keys into your backup. The server will however never be able to read your messages."
                color: Nheko.colors.text
                wrapMode: Text.Wrap
            }

            Item {
                Layout.margins: Nheko.paddingMedium
                Layout.preferredHeight: storeSecretsOnline.height
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.fillWidth: true

                ToggleButton {
                    id: useOnlineKeyBackup

                    checked: true
                    onClicked: console.log("Online key backup toggled: " + checked)
                }

            }

        }

        background: Rectangle {
            color: Nheko.colors.window
            border.color: Nheko.theme.separator
            border.width: 1
            radius: Nheko.paddingSmall
        }

    }

    MainWindowDialog {
        id: verifyMasterKey

        standardButtons: Dialog.Cancel

        GridLayout {
            id: masterGrid

            width: verifyMasterKey.useableWidth
            columns: 1
            z: 1

            Label {
                Layout.margins: Nheko.paddingMedium
                Layout.alignment: Qt.AlignHCenter
                //Layout.columnSpan: 2
                font.pointSize: fontMetrics.font.pointSize * 2
                text: qsTr("Activate Encryption")
                color: Nheko.colors.text
                wrapMode: Text.Wrap
            }

            Label {
                Layout.margins: Nheko.paddingMedium
                Layout.alignment: Qt.AlignLeft
                //Layout.columnSpan: 2
                Layout.maximumWidth: grid.width - Nheko.paddingMedium * 2
                text: qsTr("It seems like you have encryption already configured for this account. To be able to access your encrypted messages and make this device appear as trusted, you can either verify an existing device or (if you have one) enter your recovery passphrase. Please select one of the options below.\nIf you choose verify, you need to have the other device available. If you choose \"enter passphrase\", you will need your recovery key or passphrase. If you click cancel, you can choose to verify yourself at a later point.")
                color: Nheko.colors.text
                wrapMode: Text.Wrap
            }

            FlatButton {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("verify")
                onClicked: {
                    SelfVerificationStatus.verifyMasterKey();
                    verifyMasterKey.close();
                }
            }

            FlatButton {
                visible: SelfVerificationStatus.hasSSSS
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("enter passphrase")
                onClicked: {
                    SelfVerificationStatus.verifyMasterKeyWithPassphrase();
                    verifyMasterKey.close();
                }
            }

        }

    }

    Connections {
        function onStatusChanged() {
            console.log("STATUS CHANGED: " + SelfVerificationStatus.status);
            if (SelfVerificationStatus.status == SelfVerificationStatus.NoMasterKey) {
                bootstrapCrosssigning.open();
            } else if (SelfVerificationStatus.status == SelfVerificationStatus.UnverifiedMasterKey) {
                verifyMasterKey.open();
            } else {
                bootstrapCrosssigning.close();
                verifyMasterKey.close();
            }
        }

        function onShowRecoveryKey(key) {
            showRecoverKeyDialog.recoveryKey = key;
            showRecoverKeyDialog.open();
        }

        function onSetupCompleted() {
            successDialog.open();
        }

        function onSetupFailed(m) {
            failureDialog.errorMessage = m;
            failureDialog.open();
        }

        target: SelfVerificationStatus
    }

}
