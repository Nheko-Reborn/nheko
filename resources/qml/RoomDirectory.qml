// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko 1.0

ApplicationWindow {
    id: roomDirectoryWindow
    visible: true

    width: Math.round(parent.width / 2)
    height: 0.75 * parent.height

    menuBar: MenuBar {
    }

    header: ToolBar {

    }

    footer: TabBar {

    }

    StackView {
        
        anchors.fill: parent
        
    }

    function joinRoom(index) {
        // body...
    }
    
    function previewRoon(index) {
        // body...
    }
}