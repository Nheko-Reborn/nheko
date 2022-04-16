// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko

Rectangle {
    color: "#2ECC71"
    implicitHeight: visible ? rowLayout.height + 8 : 0
    visible: CallManager.haveCallInvite && !Settings.mobileMode

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
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        Avatar {
            displayName: CallManager.callPartyDisplayName
            height: Nheko.avatarSize
            url: CallManager.callPartyAvatarUrl.replace("mxc://", "image://MxcImage/")
            userid: CallManager.callParty
            width: Nheko.avatarSize

            onClicked: TimelineManager.openImageOverlay(room, room.avatarUrl(userid), room.data.eventId)
        }
        Label {
            Layout.leftMargin: 8
            color: "#000000"
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: CallManager.callPartyDisplayName
        }
        Image {
            Layout.leftMargin: 4
            Layout.preferredHeight: 24
            Layout.preferredWidth: 24
            source: CallManager.callType == Voip.CallType.VIDEO ? "qrc:/icons/icons/ui/video.svg" : "qrc:/icons/icons/ui/place-call.svg"
        }
        Label {
            color: "#000000"
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: CallManager.callType == Voip.VIDEO ? qsTr("Video Call") : qsTr("Voice Call")
        }
        Item {
            Layout.fillWidth: true
        }
        ImageButton {
            Layout.rightMargin: 16
            ToolTip.text: qsTr("Devices")
            ToolTip.visible: hovered
            buttonTextColor: "#000000"
            height: 20
            hoverEnabled: true
            image: ":/icons/icons/ui/settings.svg"
            width: 20

            onClicked: {
                var dialog = devicesDialog.createObject(timelineRoot);
                dialog.open();
                timelineRoot.destroyOnClose(dialog);
            }
        }
        Button {
            Layout.rightMargin: 4
            icon.source: CallManager.callType == Voip.VIDEO ? "qrc:/icons/icons/ui/video.svg" : "qrc:/icons/icons/ui/place-call.svg"
            palette: timelineRoot.palette
            text: qsTr("Accept")

            onClicked: {
                if (CallManager.mics.length == 0) {
                    var dialog = deviceError.createObject(timelineRoot, {
                            "errorString": qsTr("No microphone found."),
                            "image": ":/icons/icons/ui/place-call.svg"
                        });
                    dialog.open();
                    timelineRoot.destroyOnClose(dialog);
                    return;
                } else if (!CallManager.mics.includes(Settings.microphone)) {
                    var dialog = deviceError.createObject(timelineRoot, {
                            "errorString": qsTr("Unknown microphone: %1").arg(Settings.microphone),
                            "image": ":/icons/icons/ui/place-call.svg"
                        });
                    dialog.open();
                    timelineRoot.destroyOnClose(dialog);
                    return;
                }
                if (CallManager.callType == Voip.VIDEO && CallManager.cameras.length > 0 && !CallManager.cameras.includes(Settings.camera)) {
                    var dialog = deviceError.createObject(timelineRoot, {
                            "errorString": qsTr("Unknown camera: %1").arg(Settings.camera),
                            "image": ":/icons/icons/ui/video.svg"
                        });
                    dialog.open();
                    timelineRoot.destroyOnClose(dialog);
                    return;
                }
                CallManager.acceptInvite();
            }
        }
        Button {
            Layout.rightMargin: 16
            icon.source: "qrc:/icons/icons/ui/end-call.svg"
            palette: timelineRoot.palette
            text: qsTr("Decline")

            onClicked: {
                CallManager.hangUp();
            }
        }
    }
}
