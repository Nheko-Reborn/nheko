import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.3
import im.nheko 1.0

ApplicationWindow {
	id: roomSettingsDialog

    property var roomSettings

	x: MainWindow.x + (MainWindow.width / 2) - (width / 2)
    y: MainWindow.y + (MainWindow.height / 2) - (height / 2)
    height: 600
    width: 420
    minimumHeight: 420
    palette: colors
    color: colors.window
    title: roomSettings.roomName
    modality: Qt.Modal

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: roomSettingsDialog.close()
    }

    ColumnLayout {
        id: contentLayout

        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Avatar {
            url: ""
            height: 130
            width: 130
            displayName: ""
            userid: ""
            Layout.alignment: Qt.AlignHCenter
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter

            MatrixText {
                text: "room name"
                font.pixelSize: 24
                Layout.alignment: Qt.AlignHCenter
            }

            MatrixText {
                text: "1 member"
                Layout.alignment: Qt.AlignHCenter
            }
        }

        ImageButton {
            Layout.alignment: Qt.AlignHCenter
            image: ":/icons/icons/ui/edit.png"
        }

        MatrixText {
            text: "SETTINGS"
        }

        RowLayout {
            MatrixText {
                text: "Notifications"
            }

            Item {
                Layout.fillWidth: true
            }

            ComboBox {
                model: [ "Muted", "Mentions only", "All messages" ]
            }
        }

        RowLayout {
            MatrixText {
                text: "Room access"
            }

            ComboBox {
                Layout.fillWidth: true
                model: [ "Anyone and guests", "Anyone", "Invited users" ]
            }
        }

        RowLayout {
            MatrixText {
                text: "Encryption"
            }

            Item {
                Layout.fillWidth: true
            }

            Switch {
            }
        }

        RowLayout {
            MatrixText {
                text: "Respond to key requests"
            }

            Item {
                Layout.fillWidth: true
            }

            Switch {
            }
        }

        MatrixText {
            text: "INFO"
        }

        RowLayout {
            MatrixText {
                text: "Internal ID"
            }

            Item {
                Layout.fillWidth: true
            }

            MatrixText {
                text: "asdajdhasjkdhaskjdhasjdks"
                font.pixelSize: 12
            }
        }

        RowLayout {
            MatrixText {
                text: "Room Version"
            }

            Item {
                Layout.fillWidth: true
            }

            MatrixText {
                text: "6"
                font.pixelSize: 12
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "Ok"
        }
    }
}