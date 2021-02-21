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

        frameRateCombo.currentIndex = frameRateCombo.find(Settings.screenShareFrameRate);
        pipCheckBox.checked = Settings.screenSharePiP;
        remoteVideoCheckBox.checked = Settings.screenShareRemoteVideo;
        hideCursorCheckBox.checked = Settings.screenShareHideCursor;
    }
    palette: colors

    ColumnLayout {
        Label {
            Layout.topMargin: 16
            Layout.bottomMargin: 16
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Share desktop with %1?").arg(TimelineManager.timeline.roomName)
            color: colors.windowText
        }

        RowLayout {
            Layout.leftMargin: 8
            Layout.rightMargin: 8

            Label {
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Frame rate:")
                color: colors.windowText
            }

            ComboBox {
                id: frameRateCombo

                Layout.alignment: Qt.AlignRight
                model: ["25", "20", "15", "10", "5", "2", "1"]
            }

        }

        CheckBox {
            id: pipCheckBox

            visible: CallManager.cameras.length > 0
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            text: qsTr("Include your camera picture-in-picture")
        }

        CheckBox {
            id: remoteVideoCheckBox

            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            text: qsTr("Request remote camera")
            ToolTip.text: qsTr("View your callee's camera like a regular video call")
            ToolTip.visible: hovered
        }

        CheckBox {
            id: hideCursorCheckBox

            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            text: qsTr("Hide mouse cursor")
        }

        RowLayout {
            Layout.margins: 8

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Share")
                icon.source: "qrc:/icons/icons/ui/screen-share.png"
                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText;
                        Settings.screenShareFrameRate = frameRateCombo.currentText;
                        Settings.screenSharePiP = pipCheckBox.checked;
                        Settings.screenShareRemoteVideo = remoteVideoCheckBox.checked;
                        Settings.screenShareHideCursor = hideCursorCheckBox.checked;
                        CallManager.sendInvite(TimelineManager.timeline.roomId(), CallType.SCREEN);
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

    }

    background: Rectangle {
        color: colors.window
        border.color: colors.windowText
    }

}
