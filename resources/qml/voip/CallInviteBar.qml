import "../"
import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10
import im.nheko 1.0

Rectangle {
    visible: CallManager.haveCallInvite
    color: "#2ECC71"
    implicitHeight: visible ? rowLayout.height + 8 : 0

    Component {
        id: devicesDialog

        CallDevices {
        }

    }

    Component {
        id: deviceError

        DeviceError {
        }

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
            onClicked: TimelineManager.openImageOverlay(TimelineManager.timeline.avatarUrl(userid), TimelineManager.timeline.data.id)
        }

        Label {
            Layout.leftMargin: 8
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: CallManager.callParty
            color: "#000000"
        }

        Image {
            Layout.leftMargin: 4
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            source: CallManager.isVideo ? "qrc:/icons/icons/ui/video-call.png" : "qrc:/icons/icons/ui/place-call.png"
        }

        Label {
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: CallManager.isVideo ? qsTr("Video Call") : qsTr("Voice Call")
            color: "#000000"
        }

        Item {
            Layout.fillWidth: true
        }

        ImageButton {
            Layout.rightMargin: 16
            width: 20
            height: 20
            buttonTextColor: "#000000"
            image: ":/icons/icons/ui/settings.png"
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Devices")
            onClicked: {
                CallManager.refreshDevices();
                var dialog = devicesDialog.createObject(timelineRoot);
                dialog.open();
            }
        }

        Button {
            Layout.rightMargin: 4
            icon.source: CallManager.isVideo ? "qrc:/icons/icons/ui/video-call.png" : "qrc:/icons/icons/ui/place-call.png"
            text: qsTr(" Accept ")
            palette: colors
            onClicked: {
                if (CallManager.mics.length == 0) {
                    var dialog = deviceError.createObject(timelineRoot, {
                        "errorString": qsTr("No microphone found."),
                        "image": ":/icons/icons/ui/place-call.png"
                    });
                    dialog.open();
                    return ;
                } else if (!CallManager.mics.includes(Settings.microphone)) {
                    var dialog = deviceError.createObject(timelineRoot, {
                        "errorString": qsTr("Unknown microphone: ") + Settings.microphone,
                        "image": ":/icons/icons/ui/place-call.png"
                    });
                    dialog.open();
                    return ;
                }
                if (CallManager.isVideo && CallManager.cameras.length > 0 && !CallManager.cameras.includes(Settings.camera)) {
                    var dialog = deviceError.createObject(timelineRoot, {
                        "errorString": qsTr("Unknown camera: ") + Settings.camera,
                        "image": ":/icons/icons/ui/video-call.png"
                    });
                    dialog.open();
                    return ;
                }
                CallManager.acceptInvite();
            }
        }

        Button {
            Layout.rightMargin: 16
            icon.source: "qrc:/icons/icons/ui/end-call.png"
            text: qsTr(" Decline ")
            palette: colors
            onClicked: {
                CallManager.hangUp();
            }
        }

    }

}
