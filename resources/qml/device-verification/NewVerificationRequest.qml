import QtQuick 2.3
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.10
import im.nheko 1.0

Pane {
    property string title: flow.sender ? qsTr("Send Device Verification Request") : qsTr("Recieved Device Verification Request")

    ColumnLayout {
        spacing: 16

        Label {
            Layout.maximumWidth: 400
            Layout.fillHeight: true
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: flow.sender ? qsTr("To ensure that no malicious user can eavesdrop on your encrypted communications, you can verify this device.") : qsTr("The device was requested to be verified")
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
