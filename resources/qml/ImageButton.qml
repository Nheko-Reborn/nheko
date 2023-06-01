// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./ui"
import QtQuick 2.3
import QtQuick.Controls 2.3
import im.nheko 1.0 // for cursor shape

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
        source: image != "" ? ("image://colorimage/" + image + "?" + ((button.hovered && changeColorOnHover) ? highlightColor : buttonTextColor)) : ""
        sourceSize.height: button.height
        sourceSize.width: button.width
    }
    CursorShape {
        id: mouseArea

        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }
    Ripple {
        color: Qt.rgba(buttonTextColor.r, buttonTextColor.g, buttonTextColor.b, 0.5)
        enabled: button.ripple
    }
}
