// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Popup {
    id: callInv

    closePolicy: Popup.NoAutoClose
    width: parent.width
    height: parent.height
    palette: Nheko.colors

    Component {
        id: deviceError

        DeviceError {
        }

    }

    Connections {
        function onNewInviteState() {
            if (!CallManager.haveCallInvite)
                close();

        }

        target: CallManager
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        Label {
            Layout.alignment: Qt.AlignCenter
            Layout.topMargin: callInv.parent.height / 25
            Layout.fillWidth: true
            text: CallManager.callPartyDisplayName
            font.pointSize: fontMetrics.font.pointSize * 2
            color: Nheko.colors.windowText
            horizontalAlignment: Text.AlignHCenter
        }

        Avatar {
            Layout.alignment: Qt.AlignCenter
            Layout.preferredHeight: callInv.height / 5
            Layout.preferredWidth: callInv.height / 5
            url: CallManager.callPartyAvatarUrl.replace("mxc://", "image://MxcImage/")
            userid: CallManager.callParty
            displayName: CallManager.callPartyDisplayName
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignCenter
            Layout.bottomMargin: callInv.height / 25

            Image {
                property string image: CallManager.callType == CallType.VIDEO ? ":/icons/icons/ui/video.svg" : ":/icons/icons/ui/place-call.svg"

                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: callInv.height / 10
                Layout.preferredHeight: callInv.height / 10
                source: "image://colorimage/" + image + "?" + Nheko.colors.windowText
            }

            Label {
                Layout.alignment: Qt.AlignCenter
                text: CallManager.callType == CallType.VIDEO ? qsTr("Video Call") : qsTr("Voice Call")
                font.pointSize: fontMetrics.font.pointSize * 2
                color: Nheko.colors.windowText
            }

        }

        ColumnLayout {
            id: deviceCombos

            property int imageSize: callInv.height / 20

            Layout.alignment: Qt.AlignCenter
            Layout.bottomMargin: callInv.height / 25

            RowLayout {
                Layout.alignment: Qt.AlignCenter

                Image {
                    Layout.preferredWidth: deviceCombos.imageSize
                    Layout.preferredHeight: deviceCombos.imageSize
                    source: "image://colorimage/:/icons/icons/ui/microphone-unmute.svg?" + Nheko.colors.windowText
                }

                ComboBox {
                    id: micCombo

                    Layout.fillWidth: true
                    model: CallManager.mics
                }

            }

            RowLayout {
                visible: CallManager.callType == CallType.VIDEO && CallManager.cameras.length > 0
                Layout.alignment: Qt.AlignCenter

                Image {
                    Layout.preferredWidth: deviceCombos.imageSize
                    Layout.preferredHeight: deviceCombos.imageSize
                    source: "image://colorimage/:/icons/icons/ui/video.svg?" + Nheko.colors.windowText
                }

                ComboBox {
                    id: cameraCombo

                    Layout.fillWidth: true
                    model: CallManager.cameras
                }

            }

        }

        RowLayout {
            id: buttonLayout

            property int buttonSize: callInv.height / 8

            function validateMic() {
                if (CallManager.mics.length == 0) {
                    var dialog = deviceError.createObject(timelineRoot, {
                        "errorString": qsTr("No microphone found."),
                        "image": ":/icons/icons/ui/place-call.svg"
                    });
                    dialog.open();
                    timelineRoot.destroyOnClose(dialog);
                    return false;
                }
                return true;
            }

            Layout.alignment: Qt.AlignCenter
            spacing: callInv.height / 6

            RoundButton {
                implicitWidth: buttonLayout.buttonSize
                implicitHeight: buttonLayout.buttonSize
                onClicked: {
                    CallManager.hangUp();
                    close();
                }

                background: Rectangle {
                    radius: buttonLayout.buttonSize / 2
                    color: "#ff0000"
                }

                contentItem: Image {
                    source: "image://colorimage/:/icons/icons/ui/end-call.svg?#ffffff"
                }

            }

            RoundButton {
                id: acceptButton

                property string image: CallManager.callType == CallType.VIDEO ? ":/icons/icons/ui/video.svg" : ":/icons/icons/ui/place-call.svg"

                implicitWidth: buttonLayout.buttonSize
                implicitHeight: buttonLayout.buttonSize
                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText;
                        if (cameraCombo.visible)
                            Settings.camera = cameraCombo.currentText;

                        CallManager.acceptInvite();
                        close();
                    }
                }

                background: Rectangle {
                    radius: buttonLayout.buttonSize / 2
                    color: "#00ff00"
                }

                contentItem: Image {
                    source: "image://colorimage/" + acceptButton.image + "?#ffffff"
                }

            }

        }

    }

    background: Rectangle {
        color: Nheko.colors.window
        border.color: Nheko.colors.windowText
    }

}
