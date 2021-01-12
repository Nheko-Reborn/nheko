import QtQuick 2.3
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.10
import im.nheko 1.0

Pane {
    property string title: flow.sender ? qsTr("Send Verification Request") : qsTr("Recieved Verification Request")

    ColumnLayout {
        spacing: 16

        Label {
            // Self verification

            Layout.maximumWidth: 400
            Layout.fillHeight: true
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: {
                if (flow.sender) {
                    if (flow.isSelfVerification)
                        return qsTr("To allow other users to see, which of your devices actually belong to you, you can verify them. This also allows key backup to work automatically. Verify %1 now?").arg(flow.deviceId);
                    else
                        return qsTr("To ensure that no malicious user can eavesdrop on your encrypted communications you can verify the other party.");
                } else {
                    if (!flow.isSelfVerification && flow.isDeviceVerification)
                        return qsTr("%1 has requested to verify their device %2.").arg(flow.userId).arg(flow.deviceId);
                    else if (!flow.isSelfVerification && !flow.isDeviceVerification)
                        return qsTr("%1 using the device %2 has requested to be verified.").arg(flow.userId).arg(flow.deviceId);
                    else
                        return qsTr("Your device (%1) has requested to be verified.").arg(flow.deviceId);
                }
            }
            color: colors.text
            verticalAlignment: Text.AlignVCenter
        }

        RowLayout {
            Button {
                Layout.alignment: Qt.AlignLeft
                text: flow.sender ? qsTr("Cancel") : qsTr("Deny")
                onClicked: {
                    flow.cancel();
                    dialog.close();
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: flow.sender ? qsTr("Start verification") : qsTr("Accept")
                onClicked: flow.next()
            }

        }

    }

}
