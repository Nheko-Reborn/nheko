// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../ui"
import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko // for cursor shape

AbstractButton {
    id: button

    property color buttonTextColor: timelineRoot.palette.placeholderText
    property alias cursor: mouseArea.cursorShape
    property color highlightColor: timelineRoot.palette.highlight

    focusPolicy: Qt.NoFocus
    height: buttonText.implicitHeight
    implicitHeight: buttonText.implicitHeight
    implicitWidth: buttonText.implicitWidth
    width: buttonText.implicitWidth

    Label {
        id: buttonText
        anchors.centerIn: parent
        color: button.hovered ? highlightColor : buttonTextColor
        font: button.font
        horizontalAlignment: Text.AlignHCenter
        padding: 0
        text: button.text
        verticalAlignment: Text.AlignVCenter
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
