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

    ColumnLayout {
        id: columnLayout

        spacing: 16

        ColumnLayout {
            spacing: 8

            RowLayout {

                Layout.topMargin: 8
                Layout.leftMargin: 8
                Layout.rightMargin: 8

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

        RowLayout {

            Layout.rightMargin: 8
            Layout.bottomMargin: 8

            Item {
                implicitWidth: 128
            }

            Button {
                text: qsTr("Ok")
                onClicked: {
                      Settings.microphone = micCombo.currentText
                      if (cameraCombo.visible) {
                          Settings.camera = cameraCombo.currentText
                      }
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
    }
}
