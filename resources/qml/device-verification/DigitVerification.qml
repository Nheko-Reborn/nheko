// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10

ColumnLayout {
    property string title: qsTr("Verification Code")

    spacing: 16

    Label {
        Layout.fillWidth: true
        Layout.preferredWidth: 400
        color: palette.text
        text: qsTr("Please verify the following digits. You should see the same numbers on both sides. If they differ, please press 'They do not match!' to abort verification!")
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
    }
    Item {
        Layout.fillHeight: true
    }
    RowLayout {
        Layout.alignment: Qt.AlignHCenter

        Label {
            color: palette.text
            font.pixelSize: Qt.application.font.pixelSize * 2
            text: flow.sasList[0]
        }
        Label {
            color: palette.text
            font.pixelSize: Qt.application.font.pixelSize * 2
            text: flow.sasList[1]
        }
        Label {
            color: palette.text
            font.pixelSize: Qt.application.font.pixelSize * 2
            text: flow.sasList[2]
        }
    }
    Item {
        Layout.fillHeight: true
    }
    RowLayout {
        Button {
            Layout.alignment: Qt.AlignLeft
            text: qsTr("They do not match!")

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
            text: qsTr("They match!")

            onClicked: flow.next()
        }
    }
}
