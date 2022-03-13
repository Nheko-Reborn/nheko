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
    modal: true
    // only set the anchors on Qt 5.12 or higher
    // see https://doc.qt.io/qt-5/qml-qtquick-controls2-popup.html#anchors.centerIn-prop
    Component.onCompleted: {
        if (anchors)
            anchors.centerIn = parent;

    }
    palette: Nheko.colors

    Component {
        id: deviceError

        DeviceError {
        }

    }

    ColumnLayout {
        id: columnLayout

        spacing: 16

        RowLayout {
            Layout.topMargin: 8
            Layout.leftMargin: 8

            Label {
                text: qsTr("Place a call to %1?").arg(room.roomName)
                color: Nheko.colors.windowText
            }

            Item {
                Layout.fillWidth: true
            }

        }

        RowLayout {
            id: buttonLayout

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

            Layout.leftMargin: 8
            Layout.rightMargin: 8

            Avatar {
                Layout.rightMargin: cameraCombo.visible ? 16 : 64
                width: Nheko.avatarSize
                height: Nheko.avatarSize
                url: room.roomAvatarUrl.replace("mxc://", "image://MxcImage/")
                displayName: room.roomName
                roomid: room.roomId
                onClicked: TimelineManager.openImageOverlay(room, room.avatarUrl(userid), room.data.eventId)
            }

            Button {
                text: qsTr("Voice")
                icon.source: "qrc:/icons/icons/ui/place-call.svg"
                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText;
                        CallManager.sendInvite(room.roomId, CallType.VOICE);
                        close();
                    }
                }
            }

            Button {
                visible: CallManager.cameras.length > 0
                text: qsTr("Video")
                icon.source: "qrc:/icons/icons/ui/video.svg"
                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText;
                        Settings.camera = cameraCombo.currentText;
                        CallManager.sendInvite(room.roomId, CallType.VIDEO);
                        close();
                    }
                }
            }

            Button {
                visible: CallManager.screenShareSupported
                text: qsTr("Screen")
                icon.source: "qrc:/icons/icons/ui/screen-share.svg"
                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText;
                        Settings.camera = cameraCombo.currentText;

                        var dialog = screenShareDialog.createObject(timelineRoot);
                        dialog.open();
                        timelineRoot.destroyOnClose(dialog);
                        close();
                    }
                }
            }

            Button {
                text: qsTr("Cancel")
                onClicked: {
                    close();
                }
            }

        }

        ColumnLayout {
            spacing: 8

            RowLayout {
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.bottomMargin: cameraCombo.visible ? 0 : 8

                Image {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    source: "image://colorimage/:/icons/icons/ui/microphone-unmute.svg?" + Nheko.colors.windowText
                }

                ComboBox {
                    id: micCombo

                    Layout.fillWidth: true
                    model: CallManager.mics
                }

            }

            RowLayout {
                visible: CallManager.cameras.length > 0
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.bottomMargin: 8

                Image {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    source: "image://colorimage/:/icons/icons/ui/video.svg?" + Nheko.colors.windowText
                }

                ComboBox {
                    id: cameraCombo

                    Layout.fillWidth: true
                    model: CallManager.cameras
                }

            }

        }

    }

    background: Rectangle {
        color: Nheko.colors.window
        border.color: Nheko.colors.windowText
    }

}
