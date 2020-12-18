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
                text: "Place a call to " + TimelineManager.timeline.roomName + "?"
            }

            Item {
                Layout.fillWidth: true
            }
        }

        RowLayout {

            id: rowLayout
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.bottomMargin: 16
            spacing: 16

            function validateMic() {
                if (CallManager.mics.length == 0) {
                    warningDialog.text = "No microphone found.";
                    warningDialog.open();
                    return false;
                }
                else if (!CallManager.mics.includes(Settings.microphone)) {
                    warningDialog.text = "Unknown microphone: " + Settings.microphone;
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

            Button {
                text: qsTr("Voice")
                icon.source: "qrc:/icons/icons/ui/place-call.png"
                onClicked: {
                    if (rowLayout.validateMic()) {
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
                    if (rowLayout.validateMic()) {
                        if (!CallManager.cameras.includes(Settings.camera)) {
                            warningDialog.text = "Unknown camera: " + Settings.camera;
                            warningDialog.open();
                            return;
                        }
                        CallManager.sendInvite(TimelineManager.timeline.roomId(), true);
                        close();
                    }
                }
            }

            Button {
                palette: colors
                text: qsTr("Cancel")
                onClicked: {
                    close();
                }
            }
        }
    }
}
