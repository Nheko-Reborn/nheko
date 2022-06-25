// SPDX-FileCopyrightText: 2022 Nheko Contributors
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
    property int maxExpansion: 400

    property string error: regis.error

    Registration {
        id: regis
    }

    ScrollView {
        id: scroll

        clip: false
        palette: Nheko.colors
        ScrollBar.horizontal.visible: false
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: Math.min(registrationPage.height, col.implicitHeight)
        anchors.margins: Nheko.paddingLarge

        contentWidth: availableWidth

        ColumnLayout {
            id: col

            spacing: Nheko.paddingMedium

            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(registrationPage.maxExpansion, scroll.width- Nheko.paddingLarge*2)

            Image {
                Layout.alignment: Qt.AlignHCenter
                source: "qrc:/logos/login.png"
                height: 128
                width: 128
            }

            RowLayout {
                spacing: Nheko.paddingLarge

                Layout.fillWidth: true
                MatrixTextField {
                    id: hsLabel
                    label: qsTr("Homeserver")
                    placeholderText: qsTr("your.server")
                    onEditingFinished: regis.setServer(text)

                    ToolTip.text: qsTr("A server that allows registration. Since matrix is decentralized, you need to first find a server you can register on or host your own.")
                }


                Spinner {
                    height: hsLabel.height/2
                    Layout.alignment: Qt.AlignBottom

                    visible: running
                    running: regis.lookingUpHs
                    foreground: Nheko.colors.mid
                }
            }

            MatrixText {
                Layout.fillWidth: true
                textFormat: Text.PlainText
                color: Nheko.theme.error
                text: regis.hsError
                visible: text
                wrapMode: TextEdit.Wrap
            }

            RowLayout {
                spacing: Nheko.paddingLarge

                visible: regis.supported

                Layout.fillWidth: true
                MatrixTextField {
                    id: usernameLabel
                    Layout.fillWidth: true
                    label: qsTr("Username")
                    ToolTip.text: qsTr("The username must not be empty, and must contain only the characters a-z, 0-9, ., _, =, -, and /.")
                    onEditingFinished: regis.checkUsername(text)
                }
                Spinner {
                    height: usernameLabel.height/2
                    Layout.alignment: Qt.AlignBottom

                    visible: running
                    running: regis.lookingUpUsername
                    foreground: Nheko.colors.mid
                }

                Image {
                    width: usernameLabel.height/2
                    height: width
                    Layout.preferredHeight: usernameLabel.height/2
                    Layout.preferredWidth: usernameLabel.height/2
                    Layout.alignment: Qt.AlignBottom
                    source: regis.usernameAvailable ? ("image://colorimage/:/icons/icons/ui/checkmark.svg?green") : ("image://colorimage/:/icons/icons/ui/dismiss.svg?"+Nheko.theme.error)
                    visible: regis.usernameAvailable || regis.usernameUnavailable
                    ToolTip.visible: ma.hovered
                    ToolTip.text: qsTr("Back")
                    sourceSize.height: height * Screen.devicePixelRatio
                    sourceSize.width: width * Screen.devicePixelRatio
                    HoverHandler {
                        id: ma
                    }
                }
            }

            MatrixText {
                Layout.fillWidth: true
                textFormat: Text.PlainText
                color: Nheko.theme.error
                text: regis.usernameError
                visible: text && regis.supported
                wrapMode: TextEdit.Wrap
            }


            MatrixTextField {
                visible: regis.supported
                id: passwordLabel
                Layout.fillWidth: true
                label: qsTr("Password")
                echoMode: TextInput.Password
                ToolTip.text: qsTr("Please choose a secure password. The exact requirements for password strength may depend on your server.")
            }

            MatrixTextField {
                visible: regis.supported
                id: passwordConfirmationLabel
                Layout.fillWidth: true
                label: qsTr("Password confirmation")
                echoMode: TextInput.Password
            }

            MatrixText {
                Layout.fillWidth: true
                visible: regis.supported
                textFormat: Text.PlainText
                color: Nheko.theme.error
                text: passwordLabel.text != passwordConfirmationLabel.text ? qsTr("Your passwords do not match!") : ""
                wrapMode: TextEdit.Wrap
            }

            MatrixTextField {
                visible: regis.supported
                id: deviceNameLabel
                Layout.fillWidth: true
                label: qsTr("Device name")
                placeholderText: regis.initialDeviceName()
                ToolTip.text: qsTr("A name for this device, which will be shown to others, when verifying your devices. If none is provided a default is used.")
            }

            Item {
                height: Nheko.avatarSize
                Layout.fillWidth: true

                Spinner {
                    height: parent.height
                    anchors.centerIn: parent

                    visible: running
                    running: regis.registering
                    foreground: Nheko.colors.mid
                }
            }

            MatrixText {
                Layout.fillWidth: true
                textFormat: Text.PlainText
                color: Nheko.theme.error
                text: registrationPage.error
                visible: text
                wrapMode: TextEdit.Wrap
            }

            FlatButton {
                id: regisBtn
                visible: regis.supported
                enabled: usernameLabel.text && passwordLabel.text && passwordLabel.text == passwordConfirmationLabel.text
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("REGISTER")
                function register() {
                    regis.startRegistration(usernameLabel.text, passwordLabel.text, deviceNameLabel.text)
                }
                onClicked: regisBtn.register()
                Keys.onEnterPressed: regisBtn.register()
                Keys.onReturnPressed: regisBtn.register()
                Keys.enabled: regisBtn.enabled && regis.supported
            }
        }
    }

    ImageButton {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: Nheko.paddingMedium
        width: Nheko.avatarSize
        height: Nheko.avatarSize
        image: ":/icons/icons/ui/angle-arrow-left.svg"
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Back")
        onClicked: mainWindow.pop()
    }
}

