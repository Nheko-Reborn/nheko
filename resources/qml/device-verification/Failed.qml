import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10
import im.nheko 1.0

Pane {
    property string title: qsTr("Verification failed")

    ColumnLayout {
        spacing: 16

        Text {
            id: content

            Layout.maximumWidth: 400
            Layout.fillHeight: true
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: {
                switch (flow.error) {
                case DeviceVerificationFlow.UnknownMethod:
                    return qsTr("Other client does not support our verification protocol.");
                case DeviceVerificationFlow.MismatchedCommitment:
                case DeviceVerificationFlow.MismatchedSAS:
                case DeviceVerificationFlow.KeyMismatch:
                    return qsTr("Key mismatch detected!");
                case DeviceVerificationFlow.Timeout:
                    return qsTr("Device verification timed out.");
                case DeviceVerificationFlow.User:
                    return qsTr("Other party canceled the verification.");
                case DeviceVerificationFlow.OutOfOrder:
                    return qsTr("Device verification timed out.");
                default:
                    return "Unknown verification error.";
                }
            }
            color: colors.text
            verticalAlignment: Text.AlignVCenter
        }

        RowLayout {
            Item {
                Layout.fillWidth: true
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Close")
                onClicked: dialog.close()
            }

        }

    }

}
