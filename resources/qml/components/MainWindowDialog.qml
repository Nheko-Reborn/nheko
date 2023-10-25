// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import im.nheko

Dialog {
    default property alias inner: scroll.data
    property int useableWidth: scroll.width - scroll.ScrollBar.vertical.width

    parent: Overlay.overlay
    anchors.centerIn: parent
    height: (Math.floor(parent.height / 2) - Nheko.paddingLarge) * 2
    width: (Math.floor(parent.width / 2) - Nheko.paddingLarge) * 2
    padding: 0
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    closePolicy: Popup.NoAutoClose
    contentChildren: [
        ScrollView {
            id: scroll

            clip: true
            anchors.fill: parent
            ScrollBar.horizontal.visible: false
            ScrollBar.vertical.visible: true
        }
    ]

    background: Rectangle {
        color: palette.window
        border.color: Nheko.theme.separator
        border.width: 1
        radius: Nheko.paddingSmall
    }

}
