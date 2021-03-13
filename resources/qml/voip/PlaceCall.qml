// SPDX-FileCopyrightText: 2021 Nheko Contributors
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
    palette: colors

    Component {
        id: deviceError

        DeviceError {
        }

    }

    Component {
        id: screenShareDialog

        ScreenShare {
        }

    }

    ColumnLayout {
        id: columnLayout

        spacing: 16

        RowLayout {
            Layout.topMargin: 8
            Layout.leftMargin: 8

            Label {
                text: qsTr("Place a call to %1?").arg(TimelineManager.timeline.roomName)
                color: colors.windowText
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
                        "image": ":/icons/icons/ui/place-call.png"
                    });
                    dialog.open();
                    return false;
                }
                return true;
            }

            Layout.leftMargin: 8
            Layout.rightMargin: 8

            Avatar {
                Layout.rightMargin: cameraCombo.visible ? 16 : 64
                width: avatarSize
                height: avatarSize
                url: TimelineManager.timeline.roomAvatarUrl.replace("mxc://", "image://MxcImage/")
                displayName: TimelineManager.timeline.roomName
                onClicked: TimelineManager.openImageOverlay(TimelineManager.timeline.avatarUrl(userid), TimelineManager.timeline.data.id)
            }

            Button {
                text: qsTr("Voice")
                icon.source: "qrc:/icons/icons/ui/place-call.png"
                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText;
                        CallManager.sendInvite(TimelineManager.timeline.roomId(), CallType.VOICE);
                        close();
                    }
                }
            }

            Button {
                visible: CallManager.cameras.length > 0
                text: qsTr("Video")
                icon.source: "qrc:/icons/icons/ui/video-call.png"
                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText;
                        Settings.camera = cameraCombo.currentText;
                        CallManager.sendInvite(TimelineManager.timeline.roomId(), CallType.VIDEO);
                        close();
                    }
                }
            }

            Button {
                visible: CallManager.screenShareSupported
                text: qsTr("Screen")
                icon.source: "qrc:/icons/icons/ui/screen-share.png"
                onClicked: {
                    var dialog = screenShareDialog.createObject(timelineRoot);
                    dialog.open();
                    close();
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
                    source: "image://colorimage/:/icons/icons/ui/microphone-unmute.png?" + colors.windowText
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
                    source: "image://colorimage/:/icons/icons/ui/video-call.png?" + colors.windowText
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
        color: colors.window
        border.color: colors.windowText
    }

}
