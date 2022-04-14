// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10

Rectangle {
    color: "red"
    implicitHeight: Qt.application.font.pixelSize * 4
    implicitWidth: col.width
    height: Qt.application.font.pixelSize * 4
    width: col.width

    ColumnLayout {
        id: col

        property var emoji: emojis.mapping[Math.floor(Math.random() * 64)]

        anchors.bottom: parent.bottom

        Label {
            height: font.pixelSize * 2
            Layout.alignment: Qt.AlignHCenter
            text: col.emoji.emoji
            font.pixelSize: Qt.application.font.pixelSize * 2
        }

        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            text: col.emoji.description
        }

    }

}
