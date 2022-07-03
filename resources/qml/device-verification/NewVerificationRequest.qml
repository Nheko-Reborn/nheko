// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10
import im.nheko 1.0

ColumnLayout {
    property string title: flow.sender ? qsTr("Send Verification Request") : qsTr("Received Verification Request")

    spacing: 16

    Label {
        // Self verification

        Layout.preferredWidth: 400
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        text: {
            if (flow.sender) {
                if (flow.isSelfVerification)
                    if (flow.isMultiDeviceVerification)
                        return qsTr("To allow other users to see, which of your devices actually belong to you, you can verify them. This also allows key backup to work automatically. Verify an unverified device now? (Please make sure you have one of those devices available.)");
                    else
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
        color: Nheko.colors.text
        verticalAlignment: Text.AlignVCenter
    }

    Item { Layout.fillHeight: true; }

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
