// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10

ColumnLayout {
    property string title: qsTr("Successful Verification")

    spacing: 16

    Label {
        id: content

        Layout.fillWidth: true
        Layout.preferredWidth: 400
        color: palette.text
        text: qsTr("Verification successful! Both sides verified their devices!")
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
    }
    Item {
        Layout.fillHeight: true
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
