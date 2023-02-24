// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Item {
    id: warningRoot

    required property string text
    required property bool isVisible

    implicitHeight: isVisible ? warningDisplay.implicitHeight + 2 * Nheko.paddingSmall : 0
    height: implicitHeight
    Layout.fillWidth: true
    Layout.margins: isVisible ? Nheko.paddingSmall : 0

    Rectangle {
        id: warningRect

        visible: warningRoot.isVisible
        // TODO: Qt.alpha() would make more sense but it wasn't working...
        color: Qt.rgba(Nheko.theme.error.r, Nheko.theme.error.g, Nheko.theme.error.b, 0.3)
        border.width: 1
        border.color: Nheko.theme.error
        radius: 3
        anchors.fill: parent
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
