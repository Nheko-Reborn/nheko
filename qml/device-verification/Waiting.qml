// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../ui"
import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10
import im.nheko

Pane {
    property string title: qsTr("Waiting for other party…")

    background: Rectangle {
        color: timelineRoot.palette.window
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            id: content
            Layout.fillWidth: true
            Layout.preferredWidth: 400
            color: timelineRoot.palette.text
            text: {
                switch (flow.state) {
                case "WaitingForOtherToAccept":
                    return qsTr("Waiting for other side to accept the verification request.");
                case "WaitingForKeys":
                    return qsTr("Waiting for other side to continue the verification process.");
                case "WaitingForMac":
                    return qsTr("Waiting for other side to complete the verification process.");
                }
            }
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
        }
        Item {
            Layout.fillHeight: true
        }
        Spinner {
            Layout.alignment: Qt.AlignHCenter
            foreground: timelineRoot.palette.mid
        }
        Item {
            Layout.fillHeight: true
        }
        RowLayout {
            Button {
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Cancel")

                onClicked: {
                    flow.cancel();
                    dialog.close();
                }
            }
            Item {
                Layout.fillWidth: true
            }
        }
    }
}
