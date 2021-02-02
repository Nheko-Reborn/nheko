import "./device-verification"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.3
import im.nheko 1.0

ApplicationWindow {
    id: userProfileDialog

    property var profile

    height: 650
    width: 420
    minimumHeight: 420
    palette: colors
    color: colors.window
    title: profile.isGlobalUserProfile ? "Global User Profile" : "Room User Profile"

    ColumnLayout {
        id: contentL

        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Avatar {
            url: profile.avatarUrl.replace("mxc://", "image://MxcImage/")
            height: 130
            width: 130
            displayName: profile.displayName
            userid: profile.userid
            Layout.alignment: Qt.AlignHCenter
            onClicked: profile.isSelf ? profile.changeAvatar() : TimelineManager.openImageOverlay(TimelineManager.timeline.avatarUrl(userid), TimelineManager.timeline.data.id)
        }

        Text {
            id: errorText
            text: "Error Text"
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

        Connections{
            target: profile
            onDisplayError: {
                errorText.opacity = 1
                hideErrorAnimation.restart()
            }
        }

        TextInput {
            id: displayUsername

            property bool isUsernameEditingAllowed

            readOnly: !isUsernameEditingAllowed
            text: profile.displayName
            font.pixelSize: 20
            color: TimelineManager.userColor(profile.userid, colors.window)
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            selectByMouse: true

            onAccepted: {
                profile.changeUsername(displayUsername.text)
                displayUsername.isUsernameEditingAllowed = false
            }

            ImageButton {
                visible: profile.isSelf
                anchors.leftMargin: 5
                anchors.left: displayUsername.right
                anchors.verticalCenter: displayUsername.verticalCenter
                image: displayUsername.isUsernameEditingAllowed ? ":/icons/icons/ui/checkmark.png" : ":/icons/icons/ui/edit.png"

                onClicked: {
                    if (displayUsername.isUsernameEditingAllowed) {
                        profile.changeUsername(displayUsername.text)
                        displayUsername.isUsernameEditingAllowed = false
                    } else {
                        displayUsername.isUsernameEditingAllowed = true
                        displayUsername.focus = true
                        displayUsername.selectAll()
                    }
                }
            }
        }

        MatrixText {
            text: profile.userid
            font.pixelSize: 15
            Layout.alignment: Qt.AlignHCenter
        }

        Button {
            id: verifyUserButton

            text: qsTr("Verify")
            Layout.alignment: Qt.AlignHCenter
            enabled: !profile.isUserVerified
            visible: !profile.isUserVerified && !profile.isSelf && profile.userVerificationEnabled
            onClicked: profile.verify()
        }

        Image {
            Layout.preferredHeight: 16
            Layout.preferredWidth: 16
            source: "image://colorimage/:/icons/icons/ui/lock.png?green"
            visible: profile.isUserVerified
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            ImageButton {
                image: ":/icons/icons/ui/do-not-disturb-rounded-sign.png"
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Ban the user")
                onClicked: profile.banUser()
            }
            // ImageButton{

            //     image:":/icons/icons/ui/volume-off-indicator.png"
            //     Layout.margins: {
            //         left: 5
            //         right: 5
            //     }
            //     ToolTip.visible: hovered
            //     ToolTip.text: qsTr("Ignore messages from this user")
            //     onClicked : {
            //         profile.ignoreUser()
            //     }
            // }
            ImageButton {
                image: ":/icons/icons/ui/black-bubble-speech.png"
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Start a private chat")
                onClicked: profile.startChat()
            }

            ImageButton {
                image: ":/icons/icons/ui/round-remove-button.png"
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Kick the user")
                onClicked: profile.kickUser()
            }

        }

        ListView {
            id: devicelist

            Layout.fillHeight: true
            Layout.minimumHeight: 200
            Layout.fillWidth: true
            clip: true
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds
            model: profile.deviceList

            delegate: RowLayout {
                width: devicelist.width
                spacing: 4

                ColumnLayout {
                    spacing: 0

                    Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft
                        elide: Text.ElideRight
                        font.bold: true
                        color: colors.text
                        text: model.deviceId
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignRight
                        elide: Text.ElideRight
                        color: colors.text
                        text: model.deviceName
                    }

                }

                Image {
                    Layout.preferredHeight: 16
                    Layout.preferredWidth: 16
                    source: ((model.verificationStatus == VerificationStatus.VERIFIED) ? "image://colorimage/:/icons/icons/ui/lock.png?green" : ((model.verificationStatus == VerificationStatus.UNVERIFIED) ? "image://colorimage/:/icons/icons/ui/unlock.png?yellow" : "image://colorimage/:/icons/icons/ui/unlock.png?red"))
                }

                Button {
                    id: verifyButton

                    visible: (!profile.userVerificationEnabled && !profile.isSelf) || (profile.isSelf && (model.verificationStatus != VerificationStatus.VERIFIED || !profile.userVerificationEnabled))
                    text: (model.verificationStatus != VerificationStatus.VERIFIED) ? "Verify" : "Unverify"
                    onClicked: {
                        if (model.verificationStatus == VerificationStatus.VERIFIED)
                            profile.unverify(model.deviceId);
                        else
                            profile.verify(model.deviceId);
                    }
                }

            }

        }

    }

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok
        onAccepted: userProfileDialog.close()
    }

}
