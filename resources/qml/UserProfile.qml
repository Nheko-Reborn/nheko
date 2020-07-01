import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.3

import im.nheko 1.0

import "./device-verification"

ApplicationWindow{
    property var user_data
    property var avatarUrl
    property var colors: currentActivePalette

    id:userProfileDialog
    height: 650
    width: 420
    modality:Qt.WindowModal
    Layout.alignment: Qt.AlignHCenter
    palette: colors

    Component {
		id: deviceVerificationDialog
		DeviceVerification {}
	}
    Component{
        id: deviceVerificationFlow
        DeviceVerificationFlow {}
    }

    background: Item{
        id: userProfileItem
        width: userProfileDialog.width
        height: userProfileDialog.height

        // Layout.fillHeight : true

        ColumnLayout{
            anchors.fill: userProfileItem
            width: userProfileDialog.width
            spacing: 10

            Avatar{
                id: userProfileAvatar
                url: avatarUrl.replace("mxc://", "image://MxcImage/")
                height: 130
                width: 130
                displayName: user_data.userName
		        userid: user_data.userId
                Layout.alignment: Qt.AlignHCenter
                Layout.margins : {
                    top: 10
                }
            }

            Label{
                id: userProfileName
                text: user_data.userName
                fontSizeMode: Text.HorizontalFit
                font.pixelSize: 20
                color:TimelineManager.userColor(user_data.userId, colors.window)
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Label{
                id: matrixUserID
                text: user_data.userId
                fontSizeMode: Text.HorizontalFit
                font.pixelSize: 15
                color:colors.text
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout{
                Layout.alignment: Qt.AlignHCenter
                ImageButton{
                    image:":/icons/icons/ui/do-not-disturb-rounded-sign.png"
                    Layout.margins: {
                        left: 5
                        right: 5
                    }
                    ToolTip.visible: hovered
			        ToolTip.text: qsTr("Ban the user")
                    onClicked : {
                        modelDeviceList.deviceList.banUser()
                    }
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
                //         modelDeviceList.deviceList.ignoreUser()
                //     }
                // }
                ImageButton{
                    image:":/icons/icons/ui/black-bubble-speech.png"
                    Layout.margins: {
                        left: 5
                        right: 5
                    }
                    ToolTip.visible: hovered
			        ToolTip.text: qsTr("Start a private chat")
                    onClicked : {
                        modelDeviceList.deviceList.startChat()
                    }
                }
                ImageButton{
                    image:":/icons/icons/ui/round-remove-button.png"
                    Layout.margins: {
                        left: 5
                        right: 5
                    }
                    ToolTip.visible: hovered
			        ToolTip.text: qsTr("Kick the user")
                    onClicked : {
                        modelDeviceList.deviceList.kickUser()
                    }
                }
            }

            ScrollView {
                implicitHeight: userProfileDialog.height/2 + 20
                implicitWidth: userProfileDialog.width-20
                clip: true
                Layout.alignment: Qt.AlignHCenter

                ListView{
                    id: devicelist
                    anchors.fill: parent
                    clip: true
                    spacing: 4

                    model: UserProfileModel{
                        id: modelDeviceList
                        deviceList.userId : user_data.userId
                    }

                    delegate: RowLayout{
                        width: parent.width
                        Layout.margins : {
                            top : 50
                        }
                        ColumnLayout{
                            RowLayout{
                                Text{
                                    Layout.fillWidth: true
                                    color: colors.text
                                    font.bold: true
                                    Layout.alignment: Qt.AlignLeft
                                    text: deviceID
                                }
                                Text{
                                    Layout.fillWidth: true
                                    color:colors.text
                                    Layout.alignment: Qt.AlignLeft
                                    text: (verified_status ==  UserProfileList.VERIFIED?"V":(verified_status ==  UserProfileList.UNVERIFIED?"NV":"B"))
                                }
                            }
                            Text{
                                Layout.fillWidth: true
                                color:colors.text
                                Layout.alignment: Qt.AlignRight
                                text: displayName
                            }
                        }
                        Button{
                            id: verifyButton
                            text:"Verify"
                            onClicked: {
                                var newFlow = deviceVerificationFlow.createObject(userProfileDialog,
                                {userId : user_data.userId,sender: true,deviceId : model.deviceID});
                                deviceVerificationList.add(newFlow.tranId);
								var dialog = deviceVerificationDialog.createObject(userProfileDialog, 
                                    {flow: newFlow});
				                dialog.show();
                            }
                            Layout.margins:{
                                right: 10
                            }
                            palette {
                                button: "white"
                            }
                            contentItem: Text {
                                text: verifyButton.text
                                color: "black"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }
            }

            Button{
                id: okbutton
                text:"OK"
                onClicked: userProfileDialog.close()
                
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom

                Layout.margins : {
                    right : 10
                    bottom: 5
                }

                palette {
                    button: "white"
                }

                contentItem: Text {
                    text: okbutton.text
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
