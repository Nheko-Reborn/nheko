// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10
import im.nheko 1.0

ColumnLayout {
    property string title: qsTr("Verification failed")
    spacing: 16

    Text {
        id: content

        Layout.preferredWidth: 400
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
                return qsTr("Verification messages received out of order!");
                default:
                return qsTr("Unknown verification error.");
            }
        }
        color: Nheko.colors.text
        verticalAlignment: Text.AlignVCenter
    }

    Item { Layout.fillHeight: true; }

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
