// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

Rectangle {
    id: r

    required property string username

    radius: fontMetrics.lineSpacing / 2 + Nheko.paddingMedium
    width: parent.width ? Math.min(parent.width, 700) : 0
    height: contents.implicitHeight + Nheko.paddingMedium * 2
    color: palette.alternateBase
    border.color: Nheko.theme.green
    border.width: 2

    RowLayout {
        id: contents

        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Image {
            source: "image://colorimage/:/icons/icons/ui/shield-filled-checkmark.svg?" + Nheko.theme.green
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
        }

        Column {
            spacing: Nheko.paddingSmall
            Layout.fillWidth: true

            MatrixText {
                text: qsTr("%1 enabled end-to-end encryption").arg(r.username)
                font.bold: true
                font.pointSize: 14
                color: palette.text
                width: parent.width
            }

            MatrixText {
                text: qsTr("Encryption keeps your messages safe by only allowing the people you sent the message to to read it. For extra security, if you want to make sure you are talking to the right people, you can verify them in real life.")
                color: palette.text
                width: parent.width
            }

        }

    }

}
