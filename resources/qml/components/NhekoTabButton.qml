// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

TabButton {
    id: control

    background: Rectangle {
        border.color: control.down ? palette.highlight : Nheko.theme.separator
        border.width: 1
        color: control.checked ? palette.highlight : palette.base
        radius: 2
    }
    contentItem: Text {
        color: control.down ? palette.highlightedText : palette.text
        elide: Text.ElideRight
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        opacity: enabled ? 1.0 : 0.3
        text: control.text
        verticalAlignment: Text.AlignVCenter
    }
}
