// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

ApplicationWindow {
    required property string eventId

    width: 400
    height: gl.implicitHeight + 2 * Nheko.paddingMedium
    title: qsTr("Report message")

    GridLayout {
        id: gl

        columnSpacing: Nheko.paddingMedium
        rowSpacing: Nheko.paddingMedium
        columns: 2
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium

        Label {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            wrapMode: Label.WordWrap
            text: qsTr("This message you are reporting will be sent to your server administrator for review. Please note that not all server administrators review reported content. You should also ask a room moderator to remove the content if necessary.")
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

            from: 0
            to: -100
            stepSize: 25
            snapMode: Slider.SnapAlways
            Layout.fillWidth: true
        }

        Item {}

        Label {
            text: {
                if (score.value === 0)
                    return qsTr("Not bad")
                else if (score.value === -25)
                    return qsTr("Mild")
                else if (score.value === -50)
                    return qsTr("Bad")
                else if (score.value === -75)
                    return qsTr("Serious")
                else if (score.value === -100)
                    return qsTr("Extremely serious")
            }
        }

        DialogButtonBox {
            Layout.columnSpan: 2
            Layout.alignment: Qt.AlignRight
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
            onAccepted: {
                room.reportEvent(eventId, reason.text, score.value);
                close();
            }
            onRejected: close()
        }
    }
}
