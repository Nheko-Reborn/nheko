// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

Control {
    id: r

    required property string userName

    Layout.fillWidth: true
    //implicitHeight: contents.implicitHeight + padd * 2
    Layout.maximumWidth: contents.Layout.maximumWidth + padding * 2
    padding: Nheko.paddingMedium

    background: Rectangle {
        border.color: Nheko.theme.green
        border.width: 2
        color: palette.alternateBase
        height: contents.implicitHeight + Nheko.paddingMedium * 2
        radius: fontMetrics.lineSpacing / 2 + Nheko.paddingMedium
    }
    contentItem: RowLayout {
        id: contents

        spacing: Nheko.paddingMedium

        Image {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 24
            Layout.preferredWidth: 24
            source: "image://colorimage/:/icons/icons/ui/shield-filled-checkmark.svg?" + Nheko.theme.green
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Nheko.paddingSmall

            MatrixText {
                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth + 1
                color: palette.text
                font.bold: true
                font.pointSize: 14
                text: qsTr("%1 enabled end-to-end encryption").arg(r.userName)
            }
            Label {
                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth + 1
                text: qsTr("Encryption keeps your messages safe by only allowing the people you sent the message to to read it. For extra security, if you want to make sure you are talking to the right people, you can verify them in real life.")
                textFormat: Text.PlainText
                wrapMode: Label.WordWrap
            }
        }
    }
}
