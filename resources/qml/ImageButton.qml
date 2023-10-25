// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound
import "./ui"
import QtQuick
import QtQuick.Controls
import im.nheko // for cursor shape

AbstractButton {
    id: button

    property color buttonTextColor: palette.buttonText
    property bool changeColorOnHover: true
    property alias cursor: mouseArea.cursorShape
    property color highlightColor: palette.highlight
    property string image: undefined
    property bool ripple: true

    focusPolicy: Qt.NoFocus
    height: 16
    width: 16

    Image {
        id: buttonImg

        // Workaround, can't get icon.source working for now...
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        source: button.image != "" ? ("image://colorimage/" + button.image + "?" + ((button.hovered && button.changeColorOnHover) ? button.highlightColor : button.buttonTextColor)) : ""
        sourceSize.height: button.height
        sourceSize.width: button.width
    }
    NhekoCursorShape {
        id: mouseArea

        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }
    Ripple {
        color: Qt.rgba(button.buttonTextColor.r, button.buttonTextColor.g, button.buttonTextColor.b, 0.5)
        enabled: button.ripple
    }
}
