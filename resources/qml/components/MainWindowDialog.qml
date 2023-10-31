// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import im.nheko

Dialog {
    default property alias inner: scroll.data
    property int useableWidth: scroll.width - scroll.ScrollBar.vertical.width

    anchors.centerIn: parent
    closePolicy: Popup.NoAutoClose
    height: (Math.floor(parent.height / 2) - Nheko.paddingLarge) * 2
    modal: true
    padding: 0
    parent: Overlay.overlay
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: (Math.floor(parent.width / 2) - Nheko.paddingLarge) * 2

    background: Rectangle {
        border.color: Nheko.theme.separator
        border.width: 1
        color: palette.window
        radius: Nheko.paddingSmall
    }
    contentChildren: [
        ScrollView {
            id: scroll

            ScrollBar.horizontal.visible: false
            ScrollBar.vertical.visible: true
            anchors.fill: parent
            clip: true
        }
    ]
}
