// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import im.nheko 1.0
import im.nheko.RoomDirectoryModel 1.0

ApplicationWindow {
    id: roomDirectoryWindow
    visible: true

    x: MainWindow.x + (MainWindow.width / 2) - (width / 2)
    y: MainWindow.y + (MainWindow.height / 2) - (height / 2)
    minimumWidth: 420
    minimumHeight: 650
    palette: colors
    color: colors.window
    modality: Qt.WindowModal
    flags: Qt.Dialog

    Component {
        id: roomDirDelegate

        Column {
            width: parent.width
            height: roomDirView.view.height
            Text {
                text: model.name + '(' + model.roomid + ')' + ': ' + model.numMembers
            }
            Text {
                text: model.topic
            }
        }
        
    }

    ListView {
        id: roomDirView
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        model: RoomDirectoryModel {}
        delegate: roomDirDelegate
    }   

    // function joinRoom(index) {
    //     // body...
    // }
    
    // function previewRoon(index) {
    //     // body...
    // }
}
