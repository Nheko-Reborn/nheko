// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

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
            width: Nheko.avatarSize
            height: Nheko.avatarSize
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
            source: CallManager.callType == CallType.VIDEO ? "qrc:/icons/icons/ui/video.svg" : "qrc:/icons/icons/ui/place-call.svg"
        }

        Label {
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: CallManager.callType == CallType.VIDEO ? qsTr("Video Call") : qsTr("Voice Call")
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
            icon.source: CallManager.callType == CallType.VIDEO ? "qrc:/icons/icons/ui/video.svg" : "qrc:/icons/icons/ui/place-call.svg"
            text: qsTr("Accept")
            palette: Nheko.colors
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
                if (CallManager.callType == CallType.VIDEO && CallManager.cameras.length > 0 && !CallManager.cameras.includes(Settings.camera)) {
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
            palette: Nheko.colors
            onClicked: {
                CallManager.hangUp();
            }
        }

    }

}
