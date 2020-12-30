import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Popup {

    modal: true
    anchors.centerIn: parent
    background: Rectangle {
        color: colors.window
        border.color: colors.windowText
    }

    // palette: colors
    // colorize controls correctly
    palette.base:             colors.base
    palette.brightText:       colors.brightText
    palette.button:           colors.button
    palette.buttonText:       colors.buttonText
    palette.dark:             colors.dark
    palette.highlight:        colors.highlight
    palette.highlightedText:  colors.highlightedText
    palette.light:            colors.light
    palette.mid:              colors.mid
    palette.text:             colors.text
    palette.window:           colors.window
    palette.windowText:       colors.windowText

    ColumnLayout {

        spacing: 16

        ColumnLayout {
            spacing: 8

            Layout.topMargin: 8
            Layout.leftMargin: 8
            Layout.rightMargin: 8

            RowLayout {

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

                visible: CallManager.isVideo && CallManager.cameras.length > 0

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

        DialogButtonBox {

            Layout.leftMargin: 128
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

            onAccepted:  {
                Settings.microphone = micCombo.currentText
                if (cameraCombo.visible) {
                    Settings.camera = cameraCombo.currentText
                }
                close();
            }

            onRejected: {
                close();
            }
        }
    }
}
