import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.2
import im.nheko 1.0
import "../"

ApplicationWindow {

    flags: Qt.Dialog
    modality: Qt.ApplicationModal
    palette: colors
    width: columnLayout.implicitWidth
    height: columnLayout.implicitHeight

    MessageDialog {
        id: warningDialog
        icon: StandardIcon.Warning
    }

    ColumnLayout {
        id: columnLayout

        spacing: 16

        RowLayout {

            Layout.topMargin: 16
            Layout.leftMargin: 8

            Label {
                font.pointSize: fontMetrics.font.pointSize * 1.1
                text: qsTr("Place a call to ") + TimelineManager.timeline.roomName + "?"
            }

            Item {
                Layout.fillWidth: true
            }
        }

        RowLayout {
            id: buttonLayout

            Layout.leftMargin: 8
            Layout.rightMargin: 8

            function validateMic() {
                if (CallManager.mics.length == 0) {
                    warningDialog.text = qsTr("No microphone found.");
                    warningDialog.open();
                    return false;
                }
                return true;
            }

            Avatar {
                width: avatarSize
                height: avatarSize
                url: TimelineManager.timeline.roomAvatarUrl.replace("mxc://", "image://MxcImage/")
                displayName: TimelineManager.timeline.roomName
            }

            Item {
                implicitWidth: cameraCombo.visible ? 16 : 64
            }

            Button {
                text: qsTr("Voice")
                icon.source: "qrc:/icons/icons/ui/place-call.png"
                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText
                        CallManager.sendInvite(TimelineManager.timeline.roomId(), false);
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
                        Settings.microphone = micCombo.currentText
                        Settings.camera = cameraCombo.currentText
                        CallManager.sendInvite(TimelineManager.timeline.roomId(), true);
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
                Layout.bottomMargin: cameraCombo.visible ? 0 : 16

                Image {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    source: "qrc:/icons/icons/ui/microphone-unmute.png"
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
                Layout.bottomMargin: 16

                Image {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    source: "qrc:/icons/icons/ui/video-call.png"
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
