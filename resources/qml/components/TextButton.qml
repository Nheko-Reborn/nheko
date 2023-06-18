// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../ui"
import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0 // for cursor shape

AbstractButton {
    id: button

    property alias cursor: mouseArea.cursorShape
    property color highlightColor: palette.highlight
    property color buttonTextColor: palette.buttonText

    focusPolicy: Qt.NoFocus
    width: buttonText.implicitWidth
    height: buttonText.implicitHeight
    implicitWidth: buttonText.implicitWidth
    implicitHeight: buttonText.implicitHeight

    Label {
        id: buttonText

        anchors.centerIn: parent
        padding: 0
        text: button.text
        color: button.hovered ? highlightColor : buttonTextColor
        font: button.font
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    NhekoCursorShape {
        id: mouseArea

        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

    Ripple {
        color: Qt.rgba(buttonTextColor.r, buttonTextColor.g, buttonTextColor.b, 0.5)
    }

}
