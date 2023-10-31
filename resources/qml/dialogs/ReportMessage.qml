// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

ApplicationWindow {
    required property string eventId

    height: gl.implicitHeight + 2 * Nheko.paddingMedium
    title: qsTr("Report message")
    width: 400

    GridLayout {
        id: gl

        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        columnSpacing: Nheko.paddingMedium
        columns: 2
        rowSpacing: Nheko.paddingMedium

        Label {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            text: qsTr("This message you are reporting will be sent to your server administrator for review. Please note that not all server administrators review reported content. You should also ask a room moderator to remove the content if necessary.")
            wrapMode: Label.WordWrap
        }
        Label {
            text: qsTr("Enter your reason for reporting:")
        }
        TextField {
            id: reason

            Layout.fillWidth: true
        }
        Label {
            text: qsTr("How bad is the message?")
        }
        Slider {
            id: score

            Layout.fillWidth: true
            from: 0
            snapMode: Slider.SnapAlways
            stepSize: 25
            to: -100
        }
        Item {
        }
        Label {
            text: {
                if (score.value === 0)
                    return qsTr("Not bad");
                else if (score.value === -25)
                    return qsTr("Mild");
                else if (score.value === -50)
                    return qsTr("Bad");
                else if (score.value === -75)
                    return qsTr("Serious");
                else if (score.value === -100)
                    return qsTr("Extremely serious");
            }
        }
        DialogButtonBox {
            Layout.alignment: Qt.AlignRight
            Layout.columnSpan: 2
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

            onAccepted: {
                room.reportEvent(eventId, reason.text, score.value);
                close();
            }
            onRejected: close()
        }
    }
}
