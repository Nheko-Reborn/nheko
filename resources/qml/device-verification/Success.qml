// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10

Pane {
    property string title: qsTr("Successful Verification")

    ColumnLayout {
        spacing: 16

        Label {
            id: content

            Layout.maximumWidth: 400
            Layout.fillHeight: true
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("Verification successful! Both sides verified their devices!")
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
