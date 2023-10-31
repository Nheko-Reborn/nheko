// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import QtQuick.Window 2.15
import im.nheko 1.0
import "../components/"
import "../ui/"
import "../"

Item {
    id: registrationPage

    property string error: regis.error
    property int maxExpansion: 400

    Registration {
        id: regis

    }
    ScrollView {
        id: scroll

        ScrollBar.horizontal.visible: false
        anchors.left: parent.left
        anchors.margins: Nheko.paddingLarge
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        clip: false
        contentWidth: availableWidth
        height: Math.min(registrationPage.height, col.implicitHeight)

        ColumnLayout {
            id: col

            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Nheko.paddingMedium
            width: Math.min(registrationPage.maxExpansion, scroll.width - Nheko.paddingLarge * 2)

            Image {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredHeight: 128
                Layout.preferredWidth: 128
                source: "qrc:/logos/login.png"
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Nheko.paddingLarge

                MatrixTextField {
                    id: hsLabel

                    ToolTip.text: qsTr("A server that allows registration. Since matrix is decentralized, you need to first find a server you can register on or host your own.")
                    label: qsTr("Homeserver")
                    placeholderText: qsTr("your.server")

                    onEditingFinished: regis.setServer(text)
                }
                Spinner {
                    Layout.alignment: Qt.AlignBottom
                    Layout.preferredHeight: hsLabel.height / 2
                    foreground: palette.mid
                    running: regis.lookingUpHs
                    visible: running
                }
            }
            MatrixText {
                Layout.fillWidth: true
                color: Nheko.theme.error
                text: regis.hsError
                textFormat: Text.PlainText
                visible: text
                wrapMode: TextEdit.Wrap
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Nheko.paddingLarge
                visible: regis.supported

                MatrixTextField {
                    id: usernameLabel

                    Layout.fillWidth: true
                    ToolTip.text: qsTr("The username must not be empty, and must contain only the characters a-z, 0-9, ., _, =, -, and /.")
                    label: qsTr("Username")

                    onEditingFinished: regis.checkUsername(text)
                }
                Spinner {
                    Layout.alignment: Qt.AlignBottom
                    Layout.preferredHeight: usernameLabel.height / 2
                    foreground: palette.mid
                    running: regis.lookingUpUsername
                    visible: running
                }
                Image {
                    Layout.alignment: Qt.AlignBottom
                    Layout.preferredHeight: usernameLabel.height / 2
                    Layout.preferredWidth: usernameLabel.height / 2
                    ToolTip.text: qsTr("Back")
                    ToolTip.visible: ma.hovered
                    source: regis.usernameAvailable ? ("image://colorimage/:/icons/icons/ui/checkmark.svg?green") : ("image://colorimage/:/icons/icons/ui/dismiss.svg?" + Nheko.theme.error)
                    sourceSize.height: height * Screen.devicePixelRatio
                    sourceSize.width: width * Screen.devicePixelRatio
                    visible: regis.usernameAvailable || regis.usernameUnavailable

                    HoverHandler {
                        id: ma

                    }
                }
            }
            MatrixText {
                Layout.fillWidth: true
                color: Nheko.theme.error
                text: regis.usernameError
                textFormat: Text.PlainText
                visible: text && regis.supported
                wrapMode: TextEdit.Wrap
            }
            MatrixTextField {
                id: passwordLabel

                Layout.fillWidth: true
                ToolTip.text: qsTr("Please choose a secure password. The exact requirements for password strength may depend on your server.")
                echoMode: TextInput.Password
                label: qsTr("Password")
                visible: regis.supported
            }
            MatrixTextField {
                id: passwordConfirmationLabel

                Layout.fillWidth: true
                echoMode: TextInput.Password
                label: qsTr("Password confirmation")
                visible: regis.supported
            }
            MatrixText {
                Layout.fillWidth: true
                color: Nheko.theme.error
                text: passwordLabel.text != passwordConfirmationLabel.text ? qsTr("Your passwords do not match!") : ""
                textFormat: Text.PlainText
                visible: regis.supported
                wrapMode: TextEdit.Wrap
            }
            MatrixTextField {
                id: deviceNameLabel

                Layout.fillWidth: true
                ToolTip.text: qsTr("A name for this device which will be shown to others when verifying your devices. If nothing is provided a default is used.")
                label: qsTr("Device name")
                placeholderText: regis.initialDeviceName()
                visible: regis.supported
            }
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: Nheko.avatarSize

                Spinner {
                    anchors.centerIn: parent
                    foreground: palette.mid
                    height: parent.height
                    running: regis.registering
                    visible: running
                }
            }
            MatrixText {
                Layout.fillWidth: true
                color: Nheko.theme.error
                text: registrationPage.error
                textFormat: Text.PlainText
                visible: text
                wrapMode: TextEdit.Wrap
            }
            FlatButton {
                id: regisBtn

                function register() {
                    regis.startRegistration(usernameLabel.text, passwordLabel.text, deviceNameLabel.text);
                }

                Keys.enabled: regisBtn.enabled && regis.supported
                Layout.alignment: Qt.AlignHCenter
                enabled: usernameLabel.text && passwordLabel.text && passwordLabel.text == passwordConfirmationLabel.text
                text: qsTr("REGISTER")
                visible: regis.supported

                Keys.onEnterPressed: regisBtn.register()
                Keys.onReturnPressed: regisBtn.register()
                onClicked: regisBtn.register()
            }
        }
    }
    ImageButton {
        ToolTip.text: qsTr("Back")
        ToolTip.visible: hovered
        anchors.left: parent.left
        anchors.margins: Nheko.paddingMedium
        anchors.top: parent.top
        height: Nheko.avatarSize
        image: ":/icons/icons/ui/angle-arrow-left.svg"
        width: Nheko.avatarSize

        onClicked: mainWindow.pop()
    }
}
