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
    id: loginPage
    property int maxExpansion: 400

    property string error: login.error

    Login {
        id: login
    }

    ScrollView {
        id: scroll

        clip: false
        palette: Nheko.colors
        ScrollBar.horizontal.visible: false
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: Math.min(loginPage.height, col.implicitHeight)
        anchors.margins: Nheko.paddingLarge

        contentWidth: availableWidth

        ColumnLayout {
            id: col

            spacing: Nheko.paddingMedium

            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(loginPage.maxExpansion, scroll.width- Nheko.paddingLarge*2)

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
                    id: matrixIdLabel
                    label: qsTr("Matrix ID")
                    placeholderText: qsTr("e.g @joe:matrix.org")
                    onEditingFinished: login.mxid = text

                    ToolTip.text: qsTr("Your login name. A mxid should start with @ followed by the user id. After the user id you need to include your server name after a :.\nYou can also put your homeserver address there, if your server doesn't support .well-known lookup.\nExample: @user:server.my\nIf Nheko fails to discover your homeserver, it will show you a field to enter the server manually.")
                    Keys.forwardTo: [pwBtn, ssoRepeater]
                }


                Spinner {
                    height: matrixIdLabel.height/2
                    Layout.alignment: Qt.AlignBottom

                    visible: running
                    running: login.lookingUpHs
                    foreground: Nheko.colors.mid
                }
            }

            MatrixText {
                Layout.fillWidth: true
                textFormat: Text.PlainText
                color: Nheko.theme.error
                text: login.mxidError
                visible: text
                wrapMode: TextEdit.Wrap
            }

            MatrixTextField {
                id: passwordLabel
                Layout.fillWidth: true
                label: qsTr("Password")
                echoMode: TextInput.Password
                ToolTip.text: qsTr("Your password.")
                visible: login.passwordSupported
                Keys.forwardTo: [pwBtn, ssoRepeater]
            }

            MatrixTextField {
                id: deviceNameLabel
                Layout.fillWidth: true
                label: qsTr("Device name")
                placeholderText: login.initialDeviceName()
                ToolTip.text: qsTr("A name for this device, which will be shown to others, when verifying your devices. If none is provided a default is used.")
                Keys.forwardTo: [pwBtn, ssoRepeater]
            }

            MatrixTextField {
                id: hsLabel
                enabled: visible
                visible: login.homeserverNeeded

                Layout.fillWidth: true
                label: qsTr("Homeserver address")
                placeholderText: qsTr("server.my:8787")
                text: login.homeserver
                onEditingFinished: login.homeserver = text
                ToolTip.text: qsTr("The address that can be used to contact you homeservers client API.\nExample: https://server.my:8787")
                Keys.forwardTo: [pwBtn, ssoRepeater]
            }

            Item {
                height: Nheko.avatarSize
                Layout.fillWidth: true

                Spinner {
                    height: parent.height
                    anchors.centerIn: parent

                    visible: running
                    running: login.loggingIn
                    foreground: Nheko.colors.mid
                }
            }

            MatrixText {
                Layout.fillWidth: true
                textFormat: Text.PlainText
                color: Nheko.theme.error
                text: loginPage.error
                visible: text
                wrapMode: TextEdit.Wrap
            }

            FlatButton {
                id: pwBtn
                visible: login.passwordSupported
                enabled: login.homeserverValid && matrixIdLabel.text == login.mxid && login.homeserver == hsLabel.text
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("LOGIN")
                function pwLogin() {
                    login.onLoginButtonClicked(Login.Password, matrixIdLabel.text, passwordLabel.text, deviceNameLabel.text)
                }
                onClicked: pwBtn.pwLogin()
                Keys.onEnterPressed: pwBtn.pwLogin()
                Keys.onReturnPressed: pwBtn.pwLogin()
                Keys.enabled: pwBtn.enabled && login.passwordSupported
            }

            Repeater {
                id: ssoRepeater

                model: login.identityProviders

                delegate: FlatButton {
                    id: ssoBtn
                    visible: login.ssoSupported
                    enabled: login.homeserverValid && matrixIdLabel.text == login.mxid && login.homeserver == hsLabel.text
                    Layout.alignment: Qt.AlignHCenter
                    text: modelData.name
                    iconImage: modelData.avatarUrl.replace("mxc://", "image://MxcImage/")
                    function ssoLogin() {
                        login.onLoginButtonClicked(Login.SSO, matrixIdLabel.text, modelData.id, deviceNameLabel.text)
                    }
                    onClicked: ssoBtn.ssoLogin()
                    Keys.onEnterPressed: ssoBtn.ssoLogin()
                    Keys.onReturnPressed: ssoBtn.ssoLogin()
                    Keys.enabled: ssoBtn.enabled && !login.passwordSupported
                }
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
