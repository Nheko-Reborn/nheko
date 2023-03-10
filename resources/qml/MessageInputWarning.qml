// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Rectangle {
    id: warningRoot

    required property string text
    property color bubbleColor: Nheko.theme.error

    implicitHeight: visible ? warningDisplay.implicitHeight + 4 * Nheko.paddingSmall : 0
    height: implicitHeight
    Layout.fillWidth: true
    color: Nheko.colors.window // required to hide the timeline behind this warning

    Rectangle {
        id: warningRect

        visible: warningRoot.visible
        // TODO: Qt.alpha() would make more sense but it wasn't working...
        color: Qt.rgba(bubbleColor.r, bubbleColor.g, bubbleColor.b, 0.3)
        border.width: 1
        border.color: bubbleColor
        radius: 3
        anchors.fill: parent
        anchors.margins: visible ? Nheko.paddingSmall : 0
        z: 3

        Label {
            id: warningDisplay

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: Nheko.paddingSmall
            color: Nheko.colors.text
            text: warningRoot.text
            textFormat: Text.PlainText
        }

    }

}
