// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Rectangle {
    id: warningRoot

    property color bubbleColor: Nheko.theme.error
    required property string text

    Layout.fillWidth: true
    color: palette.window // required to hide the timeline behind this warning
    height: implicitHeight
    implicitHeight: visible ? warningDisplay.implicitHeight + 4 * Nheko.paddingSmall : 0

    Rectangle {
        id: warningRect

        anchors.fill: parent
        anchors.margins: visible ? Nheko.paddingSmall : 0
        border.color: bubbleColor
        border.width: 1
        // TODO: Qt.alpha() would make more sense but it wasn't working...
        color: Qt.rgba(bubbleColor.r, bubbleColor.g, bubbleColor.b, 0.3)
        radius: 3
        visible: warningRoot.visible
        z: 3

        Label {
            id: warningDisplay

            anchors.left: parent.left
            anchors.margins: Nheko.paddingSmall
            anchors.verticalCenter: parent.verticalCenter
            text: warningRoot.text
            textFormat: Text.PlainText
        }
    }
}
