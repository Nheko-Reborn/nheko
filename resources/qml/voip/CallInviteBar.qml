// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

Rectangle {
    visible: CallManager.haveCallInvite && !Settings.mobileMode
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
            implicitWidth: Nheko.avatarSize
            implicitHeight: Nheko.avatarSize
            url: CallManager.callPartyAvatarUrl.replace("mxc://", "image://MxcImage/")
            userid: CallManager.callParty
            displayName: CallManager.callPartyDisplayName
            onClicked: TimelineManager.openImageOverlay(room, room.avatarUrl(userid), room.data.eventId)
        }

        Label {
            Layout.leftMargin: 8
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: CallManager.callPartyDisplayName
            color: "#000000"
        }

        Image {
            Layout.leftMargin: 4
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            source: CallManager.callType == Voip.VIDEO ? "qrc:/icons/icons/ui/video.svg" : "qrc:/icons/icons/ui/place-call.svg"
        }

        Label {
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: CallManager.callType == Voip.VIDEO ? qsTr("Video Call") : qsTr("Voice Call")
            color: "#000000"
        }

        Item {
            Layout.fillWidth: true
        }

        ImageButton {
            Layout.rightMargin: 16
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            buttonTextColor: "#000000"
            image: ":/icons/icons/ui/settings.svg"
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Devices")
            onClicked: {
                var dialog = devicesDialog.createObject(timelineRoot);
                dialog.open();
                timelineRoot.destroyOnClose(dialog);
            }
        }

        Button {
            Layout.rightMargin: 4
            icon.source: CallManager.callType == Voip.VIDEO ? "qrc:/icons/icons/ui/video.svg" : "qrc:/icons/icons/ui/place-call.svg"
            text: qsTr("Accept")
            onClicked: {
                if (CallManager.mics.length == 0) {
                    var dialog = deviceError.createObject(timelineRoot, {
                        "errorString": qsTr("No microphone found."),
                        "image": ":/icons/icons/ui/place-call.svg"
                    });
                    dialog.open();
            timelineRoot.destroyOnClose(dialog);
                    return ;
                } else if (!CallManager.mics.includes(Settings.microphone)) {
                    var dialog = deviceError.createObject(timelineRoot, {
                        "errorString": qsTr("Unknown microphone: %1").arg(Settings.microphone),
                        "image": ":/icons/icons/ui/place-call.svg"
                    });
                    dialog.open();
            timelineRoot.destroyOnClose(dialog);
                    return ;
                }
                if (CallManager.callType == Voip.VIDEO && CallManager.cameras.length > 0 && !CallManager.cameras.includes(Settings.camera)) {
                    var dialog = deviceError.createObject(timelineRoot, {
                        "errorString": qsTr("Unknown camera: %1").arg(Settings.camera),
                        "image": ":/icons/icons/ui/video.svg"
                    });
                    dialog.open();
            timelineRoot.destroyOnClose(dialog);
                    return ;
                }
                CallManager.acceptInvite();
            }
        }

        Button {
            Layout.rightMargin: 16
            icon.source: "qrc:/icons/icons/ui/end-call.svg"
            text: qsTr("Decline")
            onClicked: {
                CallManager.rejectInvite();
            }
        }

    }

}
