import "../"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Popup {
    closePolicy: Popup.NoAutoClose
    width: parent.width
    height: parent.height
    palette: colors
    background: Rectangle {
        color: colors.window
        border.color: colors.windowText
    }

    Component {
        id: deviceError
        DeviceError {
        }
    }

    Connections {
        target: CallManager
        onNewInviteState: {
            if (!CallManager.haveCallInvite) {
                close();
            }
        }
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        spacing: 48

        Item {
            Layout.fillHeight: true
        }

        Label {
            Layout.alignment: Qt.AlignCenter
            text: CallManager.callParty
            font.pointSize: fontMetrics.font.pointSize * 2
            color: colors.windowText
        }

        Avatar {
            Layout.alignment: Qt.AlignCenter
            width: avatarSize * 4
            height: avatarSize * 4
            url: CallManager.callPartyAvatarUrl.replace("mxc://", "image://MxcImage/")
            displayName: CallManager.callParty
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignCenter

            Image {
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: avatarSize
                Layout.preferredHeight: avatarSize
                property string image: CallManager.isVideo ? ":/icons/icons/ui/video-call.png" : ":/icons/icons/ui/place-call.png"
                source: "image://colorimage/" + image + "?" + colors.windowText
            }

            Label {
                Layout.alignment: Qt.AlignCenter
                text: CallManager.isVideo ? qsTr("Video Call") : qsTr("Voice Call")
                font.pointSize: fontMetrics.font.pointSize * 2
                color: colors.windowText
            }
        }

        ColumnLayout {
            id: deviceCombos
            Layout.alignment: Qt.AlignCenter
            property int imageSize: 32

            RowLayout {

                Layout.alignment: Qt.AlignCenter

                Image {
                    Layout.preferredWidth: deviceCombos.imageSize
                    Layout.preferredHeight: deviceCombos.imageSize
                    source: "image://colorimage/:/icons/icons/ui/microphone-unmute.png?" + colors.windowText
                }

                ComboBox {
                    id: micCombo
                    Layout.fillWidth: true
                    model: CallManager.mics
                }
            }

            RowLayout {

                visible: CallManager.isVideo && CallManager.cameras.length > 0
                Layout.alignment: Qt.AlignCenter

                Image {
                    Layout.preferredWidth: deviceCombos.imageSize
                    Layout.preferredHeight: deviceCombos.imageSize
                    source: "image://colorimage/:/icons/icons/ui/video-call.png?" + colors.windowText
                }

                ComboBox {
                    id: cameraCombo
                    Layout.fillWidth: true
                    model: CallManager.cameras
                }
            }
        }

        RowLayout {
            id: buttonLayout

            property int iconSize: 64
            Layout.alignment: Qt.AlignCenter
            spacing: 160

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

            RoundButton {
                icon.source: "qrc:/icons/icons/ui/end-call.png"
                icon.width: buttonLayout.iconSize
                icon.height: buttonLayout.iconSize
                icon.color: "#ffffff"
                palette.button: "#ff0000"

                onClicked: {
                    CallManager.hangUp();
                    close();
                }
            }

            RoundButton {
                icon.source: CallManager.isVideo ? "qrc:/icons/icons/ui/video-call.png" : "qrc:/icons/icons/ui/place-call.png"
                icon.width: buttonLayout.iconSize
                icon.height: buttonLayout.iconSize
                icon.color: "#ffffff"
                palette.button: "#00ff00"

                onClicked: {
                    if (buttonLayout.validateMic()) {
                        Settings.microphone = micCombo.currentText;
                        if (cameraCombo.visible)
                            Settings.camera = cameraCombo.currentText;
                        CallManager.acceptInvite();
                        close();
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
