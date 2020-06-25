import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.3

import im.nheko 1.0

import "./device-verification"

ApplicationWindow{
    property var user_data
    property var colors: currentActivePalette

    id:userProfileDialog
    height: 500
    width: 420
    modality:Qt.WindowModal
    Layout.alignment: Qt.AlignHCenter
    palette: colors

    UserProfileList{
        id: userProfileList
        userId: user_data.userId
        onUserIdChanged : {
            userProfileList.updateDeviceList()
        }
        onDeviceListUpdated : {
            modelDeviceList.deviceList = userProfileList
        }
    }

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

        Layout.fillHeight : true

        ColumnLayout{
            anchors.fill: userProfileItem
            width: userProfileDialog.width
            spacing: 10

            Avatar{
                id: userProfileAvatar
                url:chat.model.avatarUrl(user_data.userId).replace("mxc://", "image://MxcImage/")
                height: 130
                width: 130
                displayName: modelData.userName
		        userid: modelData.userId
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
                color:TimelineManager.userColor(modelData.userId, colors.window)
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

            ScrollView {
                implicitHeight: userProfileDialog.height/2+20
                implicitWidth: userProfileDialog.width-20
                clip: true
                Layout.alignment: Qt.AlignHCenter

                ListView{
                    id: deviceList
                    anchors.fill: parent
                    clip: true
                    spacing: 4

                    model: UserProfileModel{
                        id: modelDeviceList
                    } 

                    delegate: RowLayout{
                        width: parent.width
                        Layout.margins : {
                            top : 50
                        }
                        ColumnLayout{
                            Text{
                                Layout.fillWidth: true
                                color: colors.text
                                font.bold: true
                                Layout.alignment: Qt.AlignRight
                                text: deviceID
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

                Layout.margins : {
                    right : 10
                    bottom : 10
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
