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

    padding: Nheko.paddingMedium
    //implicitHeight: contents.implicitHeight + padd * 2
    Layout.maximumWidth: contents.Layout.maximumWidth + padding * 2
    Layout.fillWidth: true

    contentItem: RowLayout {
        id: contents

        spacing: Nheko.paddingMedium

        Image {
            source: "image://colorimage/:/icons/icons/ui/shield-filled-checkmark.svg?" + Nheko.theme.green
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
        }

        ColumnLayout {
            spacing: Nheko.paddingSmall
            Layout.fillWidth: true

            MatrixText {
                text: qsTr("%1 enabled end-to-end encryption").arg(r.userName)
                font.bold: true
                font.pointSize: 14
                color: palette.text
                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth + 1
            }

            Label {
                text: qsTr("Encryption keeps your messages safe by only allowing the people you sent the message to to read it. For extra security, if you want to make sure you are talking to the right people, you can verify them in real life.")
                textFormat: Text.PlainText
                wrapMode: Label.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth + 1
            }

        }

    }

    background: Rectangle {
        radius: fontMetrics.lineSpacing / 2 + Nheko.paddingMedium
        height: contents.implicitHeight + Nheko.paddingMedium * 2
        color: palette.alternateBase
        border.color: Nheko.theme.green
        border.width: 2
    }
}
