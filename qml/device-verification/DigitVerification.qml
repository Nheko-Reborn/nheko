// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10
import im.nheko 1.0

Pane {
    property string title: qsTr("Verification Code")
    background: Rectangle {
        color: timelineRoot.palette.window
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            Layout.preferredWidth: 400
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("Please verify the following digits. You should see the same numbers on both sides. If they differ, please press 'They do not match!' to abort verification!")
            color: timelineRoot.palette.text
            verticalAlignment: Text.AlignVCenter
        }

        Item { Layout.fillHeight: true; }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter

            Label {
                font.pixelSize: Qt.application.font.pixelSize * 2
                text: flow.sasList[0]
                color: timelineRoot.palette.text
            }

            Label {
                font.pixelSize: Qt.application.font.pixelSize * 2
                text: flow.sasList[1]
                color: timelineRoot.palette.text
            }

            Label {
                font.pixelSize: Qt.application.font.pixelSize * 2
                text: flow.sasList[2]
                color: timelineRoot.palette.text
            }

        }
        Item { Layout.fillHeight: true; }

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

}
