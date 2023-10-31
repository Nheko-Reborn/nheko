// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Popup {
    modal: true

    background: Rectangle {
        border.color: palette.windowText
        color: palette.window
    }

    // only set the anchors on Qt 5.12 or higher
    // see https://doc.qt.io/qt-5/qml-qtquick-controls2-popup.html#anchors.centerIn-prop
    Component.onCompleted: {
        if (anchors)
            anchors.centerIn = parent;
    }

    Component {
        id: deviceError

        DeviceError {
        }
    }
    ColumnLayout {
        id: columnLayout

        spacing: 16

        RowLayout {
            Layout.leftMargin: 8
            Layout.topMargin: 8

            Label {
                color: palette.windowText
                text: qsTr("Place a call to %1?").arg(room.roomName)
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
                Layout.preferredHeight: Nheko.avatarSize
                Layout.preferredWidth: Nheko.avatarSize
                Layout.rightMargin: cameraCombo.visible ? 16 : 64
                displayName: room.roomName
                roomid: room.roomId
                url: room.roomAvatarUrl.replace("mxc://", "image://MxcImage/")

                onClicked: TimelineManager.openImageOverlay(room, room.avatarUrl(userid), room.data.eventId)
            }
            Button {
                icon.source: "qrc:/icons/icons/ui/place-call.svg"
                text: qsTr("Voice")

                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText;
                        CallManager.sendInvite(room.roomId, Voip.VOICE);
                        close();
                    }
                }
            }
            Button {
                icon.source: "qrc:/icons/icons/ui/video.svg"
                text: qsTr("Video")
                visible: CallManager.cameras.length > 0

                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText;
                        Settings.camera = cameraCombo.currentText;
                        CallManager.sendInvite(room.roomId, Voip.VIDEO);
                        close();
                    }
                }
            }
            Button {
                icon.source: "qrc:/icons/icons/ui/screen-share.svg"
                text: qsTr("Screen")

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
                Layout.bottomMargin: cameraCombo.visible ? 0 : 8
                Layout.leftMargin: 8
                Layout.rightMargin: 8

                Image {
                    Layout.preferredHeight: 22
                    Layout.preferredWidth: 22
                    source: "image://colorimage/:/icons/icons/ui/microphone-unmute.svg?" + palette.windowText
                }
                ComboBox {
                    id: micCombo

                    Layout.fillWidth: true
                    model: CallManager.mics
                }
            }
            RowLayout {
                Layout.bottomMargin: 8
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                visible: CallManager.cameras.length > 0

                Image {
                    Layout.preferredHeight: 22
                    Layout.preferredWidth: 22
                    source: "image://colorimage/:/icons/icons/ui/video.svg?" + palette.windowText
                }
                ComboBox {
                    id: cameraCombo

                    Layout.fillWidth: true
                    model: CallManager.cameras
                }
            }
        }
    }
}
