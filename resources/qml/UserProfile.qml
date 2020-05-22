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

    onAfterRendering: {
        userProfileAvatar.url = chat.model.avatarUrl(user_data.userId).replace("mxc://", "image://MxcImage/")
        userProfileName.text = user_data.userName
        matrixUserID.text = user_data.userId
        userProfile.userId = user_data.userId
        log_devices()     
    }

    function log_devices()
    {
        console.log(userProfile.deviceList);
        userProfile.deviceList.forEach((item,index)=>{
            console.log(item.device_id)
            console.log(item.display_name)
        })
    } 

    UserProfileContent{
        id: userProfile
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
                height: 130
                width: 130
                displayName: modelData.userName
                Layout.alignment: Qt.AlignHCenter
            }

            Label{
                id: userProfileName
                fontSizeMode: Text.HorizontalFit
                Layout.alignment: Qt.AlignHCenter
            }

            Label{
                id: matrixUserID
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

                Label {
                    text: "ABC"
                    font.pixelSize: 700
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
