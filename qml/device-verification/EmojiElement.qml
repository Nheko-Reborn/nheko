// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10

Rectangle {
    color: "red"
    height: Qt.application.font.pixelSize * 4
    implicitHeight: Qt.application.font.pixelSize * 4
    implicitWidth: col.width
    width: col.width

    ColumnLayout {
        id: col

        property var emoji: emojis.mapping[Math.floor(Math.random() * 64)]

        anchors.bottom: parent.bottom

        Label {
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: Qt.application.font.pixelSize * 2
            height: font.pixelSize * 2
            text: col.emoji.emoji
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            text: col.emoji.description
        }
    }
}
