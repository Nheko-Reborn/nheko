// SPDX-FileCopyrightText: Nheko Contributors
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
    height: parent.height
    width: parent.width

    background: Rectangle {
        border.color: palette.windowText
        color: palette.window
    }

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
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top

        Label {
            Layout.alignment: Qt.AlignCenter
            Layout.fillWidth: true
            Layout.topMargin: callInv.parent.height / 25
            color: palette.windowText
            font.pointSize: fontMetrics.font.pointSize * 2
            horizontalAlignment: Text.AlignHCenter
            text: CallManager.callPartyDisplayName
        }
        Avatar {
            Layout.alignment: Qt.AlignCenter
            Layout.preferredHeight: callInv.height / 5
            Layout.preferredWidth: callInv.height / 5
            displayName: CallManager.callPartyDisplayName
            url: CallManager.callPartyAvatarUrl.replace("mxc://", "image://MxcImage/")
            userid: CallManager.callParty
        }
        ColumnLayout {
            Layout.alignment: Qt.AlignCenter
            Layout.bottomMargin: callInv.height / 25

            Image {
                property string image: CallManager.callType == Voip.VIDEO ? ":/icons/icons/ui/video.svg" : ":/icons/icons/ui/place-call.svg"

                Layout.alignment: Qt.AlignCenter
                Layout.preferredHeight: callInv.height / 10
                Layout.preferredWidth: callInv.height / 10
                source: "image://colorimage/" + image + "?" + palette.windowText
            }
            Label {
                Layout.alignment: Qt.AlignCenter
                color: palette.windowText
                font.pointSize: fontMetrics.font.pointSize * 2
                text: CallManager.callType == Voip.VIDEO ? qsTr("Video Call") : qsTr("Voice Call")
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
                    Layout.preferredHeight: deviceCombos.imageSize
                    Layout.preferredWidth: deviceCombos.imageSize
                    source: "image://colorimage/:/icons/icons/ui/microphone-unmute.svg?" + palette.windowText
                }
                ComboBox {
                    id: micCombo

                    Layout.fillWidth: true
                    model: CallManager.mics
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignCenter
                visible: CallManager.callType == Voip.VIDEO && CallManager.cameras.length > 0

                Image {
                    Layout.preferredHeight: deviceCombos.imageSize
                    Layout.preferredWidth: deviceCombos.imageSize
                    source: "image://colorimage/:/icons/icons/ui/video.svg?" + palette.windowText
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
                implicitHeight: buttonLayout.buttonSize
                implicitWidth: buttonLayout.buttonSize

                background: Rectangle {
                    color: "#ff0000"
                    radius: buttonLayout.buttonSize / 2
                }
                contentItem: Image {
                    source: "image://colorimage/:/icons/icons/ui/end-call.svg?#ffffff"
                }

                onClicked: {
                    CallManager.rejectInvite();
                    close();
                }
            }
            RoundButton {
                id: acceptButton

                property string image: CallManager.callType == Voip.VIDEO ? ":/icons/icons/ui/video.svg" : ":/icons/icons/ui/place-call.svg"

                implicitHeight: buttonLayout.buttonSize
                implicitWidth: buttonLayout.buttonSize

                background: Rectangle {
                    color: "#00ff00"
                    radius: buttonLayout.buttonSize / 2
                }
                contentItem: Image {
                    source: "image://colorimage/" + acceptButton.image + "?#ffffff"
                }

                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText;
                        if (cameraCombo.visible)
                            Settings.camera = cameraCombo.currentText;
                        CallManager.acceptInvite();
                        close();
                    }
                }
            }
        }
    }
}
