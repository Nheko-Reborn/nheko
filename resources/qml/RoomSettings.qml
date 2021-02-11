import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.3
import QtQuick.Dialogs 1.2
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
    modality: Qt.WindowModal
    flags: Qt.WindowStaysOnTopHint

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
                text: roomSettings.roomName
                font.pixelSize: 24
                Layout.alignment: Qt.AlignHCenter
            }

            MatrixText {
                text: "%1 member(s)".arg(roomSettings.memberCount)
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
                currentIndex: roomSettings.notifications
                onActivated: {
                    roomSettings.changeNotifications(index)
                }
            }
        }

        RowLayout {
            MatrixText {
                text: "Room access"
            }

            ComboBox {
                Layout.fillWidth: true
                enabled: roomSettings.canChangeJoinRules
                model: [ "Anyone and guests", "Anyone", "Invited users" ]
                currentIndex: roomSettings.accessJoinRules
                onActivated: {
                    roomSettings.changeAccessRules(index)
                }
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
                id: encryptionSwitch

                checked: roomSettings.isEncryptionEnabled
                onToggled: {
                    if(roomSettings.isEncryptionEnabled) {
                        checked=true;
                        return;
                    }

                    confirmEncryptionDialog.open();
                }
            }

            MessageDialog {
                id: confirmEncryptionDialog
                title: qsTr("End-to-End Encryption")
                text: qsTr("Encryption is currently experimental and things might break unexpectedly. <br> 
                            Please take note that it can't be disabled afterwards.")
                modality: Qt.WindowModal
                icon: StandardIcon.Question

                onAccepted: {
                    if(roomSettings.isEncryptionEnabled) {
                        return;
                    }

                    roomSettings.enableEncryption();
                }

                onRejected: {
                    encryptionSwitch.checked = false
                }

                standardButtons: Dialog.Ok | Dialog.Cancel
            }
        }

        RowLayout {
            visible: roomSettings.isEncryptionEnabled

            MatrixText {
                text: "Respond to key requests"
            }

            Item {
                Layout.fillWidth: true
            }

            Switch {
                ToolTip.text: qsTr("Whether or not the client should respond automatically with the session keys 
                                upon request. Use with caution, this is a temporary measure to test the 
                                E2E implementation until device verification is completed.")

                checked: roomSettings.respondsToKeyRequests

                onToggled: {
                    roomSettings.changeKeyRequestsPreference(checked)
                }
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
                text: roomSettings.roomId
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
                text: roomSettings.roomVersion
                font.pixelSize: 12
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "Ok"
        }
    }
}