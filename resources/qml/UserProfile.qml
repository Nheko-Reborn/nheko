import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.3

import im.nheko 1.0

ApplicationWindow{
    property var user_data
    property var colors: currentActivePalette

    id:userProfileDialog
    height: 500
    width: 500
    modality:Qt.WindowModal
    Layout.alignment: Qt.AlignHCenter
    palette: colors

    UserProfileList{
        id: userProfileList
        userId: user_data.userId
        onUserIdChanged : {
            console.log(userId)
            userProfileList.updateDeviceList()
        }
        onDeviceListUpdated : {
            modelDeviceList.deviceList = userProfileList
        }
    }

    background: Item{
        id: userProfileItem
        width: userProfileDialog.width
        height: userProfileDialog.height
        anchors.margins: {
            top:20
        }

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
                Layout.alignment: Qt.AlignHCenter
            }

            Label{
                id: userProfileName
                text: user_data.userName
                fontSizeMode: Text.HorizontalFit
                Layout.alignment: Qt.AlignHCenter
            }

            Label{
                id: matrixUserID
                text: user_data.userId
                fontSizeMode: Text.HorizontalFit
                Layout.alignment: Qt.AlignHCenter
            }

            ScrollView {
                implicitHeight: userProfileDialog.height/2+20
                implicitWidth: userProfileDialog.width-20
                clip: true
                Layout.alignment: Qt.AlignHCenter
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOn
                ScrollBar.vertical.policy: ScrollBar.AlwaysOn

                ListView{
                    id: deviceList
                    anchors.fill: parent
                    clip: true
                    spacing: 10

                    model: UserProfileModel{
                        id: modelDeviceList
                    } 

                    delegate: RowLayout{
                        width: parent.width
                        Text{
                            Layout.fillWidth: true
                            color: colors.text
                            text: deviceID
                        }
                        Text{
                            Layout.fillWidth: true
                            color:colors.text
                            text: displayName
                        }
                    }
                }
            }

            Button{
                text:"OK"
                onClicked: userProfileDialog.close()
                anchors.margins: {
                    right:10
                    bottom:10
                }

                Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            }
        }

        Item { Layout.fillHeight: true }
    }
}
