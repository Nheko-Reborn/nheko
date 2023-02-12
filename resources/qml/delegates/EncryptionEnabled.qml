// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

Rectangle {
    id: r

    required property string username

    radius: fontMetrics.lineSpacing / 2 + Nheko.paddingMedium
    width: parent.width ? parent.width : 0
    height: contents.implicitHeight + Nheko.paddingMedium * 2
    color: Nheko.colors.alternateBase
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
            width: 24
            height: width
        }

        Column {
            spacing: Nheko.paddingSmall
            Layout.fillWidth: true

            MatrixText {
                text: qsTr("%1 enabled end-to-end encryption").arg(r.username)
                font.bold: true
                font.pointSize: 14
                color: Nheko.colors.text
                width: parent.width
            }

            MatrixText {
                text: qsTr("Encryption keeps your messages safe by locking them with a key that only the people in this room have. "
                           + "That means that even if somebody gains unauthorized access to your messages, they will not be able to see "
                           + "what they say.")
                color: Nheko.colors.text
                width: parent.width
            }

        }

    }

}
