import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.2
import im.nheko 1.0
import "../"

Rectangle {

    visible: CallManager.haveCallInvite
    color: "#2ECC71"
    implicitHeight: visible ? rowLayout.height + 8 : 0

    MessageDialog {
        id: warningDialog
        icon: StandardIcon.Warning
    }

    RowLayout {
        id: rowLayout

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 8

        Avatar {
            width: avatarSize
            height: avatarSize
            url: CallManager.callPartyAvatarUrl.replace("mxc://", "image://MxcImage/")
            displayName: CallManager.callParty
        }

        Label {
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: "  " + CallManager.callParty + " "
        }

        Image {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            source: CallManager.isVideo ? "qrc:/icons/icons/ui/video-call.png" : "qrc:/icons/icons/ui/place-call.png"
        }

        Label {
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: CallManager.isVideo ? "Video Call" : "Voice Call"
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            icon.source: CallManager.isVideo ? "qrc:/icons/icons/ui/video-call.png" : "qrc:/icons/icons/ui/place-call.png"
            palette: colors
            text: qsTr("Accept")
            onClicked: {
                if (CallManager.mics.length == 0) {
                    warningDialog.text = "No microphone found.";
                    warningDialog.open();
                    return;
                }
                else if (!CallManager.mics.includes(Settings.microphone)) {
                    warningDialog.text = "Unknown microphone: " + Settings.microphone;
                    warningDialog.open();
                    return;
                }
                if (CallManager.isVideo && CallManager.cameras.length > 0 && !CallManager.cameras.includes(Settings.camera)) {
                    warningDialog.text = "Unknown camera: " + Settings.camera;
                    warningDialog.open();
                    return;
                }
                CallManager.acceptInvite();
            }
        }

        Item {
            implicitWidth: 8
        }

        Button {
            icon.source: "qrc:/icons/icons/ui/end-call.png"
            palette: colors
            text: qsTr("Decline")
            onClicked: {
                CallManager.hangUp();
            }
        }

        Item {
            implicitWidth: 16
        }
    }
}
