// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import Qt.labs.platform 1.1 as P
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import im.nheko 1.0

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
        color: Nheko.colors.window
        border.color: Nheko.theme.separator
        border.width: 1
        radius: Nheko.paddingSmall
    }

}
