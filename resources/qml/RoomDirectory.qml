// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import QtQuick.Window 2.3
import im.nheko 1.0
import im.nheko.RoomDirectoryModel 1.0

ApplicationWindow {
    id: roomDirectoryWindow
    visible: true

    x: MainWindow.x + (MainWindow.width / 2) - (width / 2)
    y: MainWindow.y + (MainWindow.height / 2) - (height / 2)
    minimumWidth: 420
    minimumHeight: 650
    palette: Nheko.colors
    color: Nheko.colors.window
    modality: Qt.WindowModal
    flags: Qt.Dialog
    title: qsTr("Explore Public Rooms")

    Rectangle {
        id: roomDirDelegateV2

        color: Nheko.colors.window
        
        anchors.fill: parent
        
        RowLayout {

            spacing: Nheko.paddingMedium
            anchors.fill: parent
            anchors.margins: Nheko.paddingMedium

            Avatar {
                id: avatar

                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: fontMetrics.lineSpacing * 2.5
                Layout.preferredHeight: fontMetrics.lineSpacing * 2.5
                url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                displayName: model.name
            }

            ColumnLayout {
                id: descriptionLines

                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                width: parent.width - avatar.width
                Layout.preferredWidth: parent.width - avatar.width
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    ElidedLabel {
                        Layout.alignment: Qt.AlignBottom
                        color: roomItem.importantText
                        elideWidth: textContent.width - timestamp.width - Nheko.paddingMedium
                        fullText: model.name
                    }

                }

            }


        }

    }

    Component {
        id: roomDirDelegate

        Column {
            width: parent.width
            height: roomDirView.view.height

            GridLayout {
                id: roomDisplay

                columns: 4 // avatar, title/topic, member count, preview/join

                rowSpacing: 10

                Text {
                    text: "Tmp"
                }

                Text {
                    text: model.name
                }

                Text {
                    text: ""
                }

                Text {
                    text: ""
                }

                Text {
                    text: ""
                }

                Text {
                    text: model.topic.substr(0, 72) + "..."
                }

                Text {
                    text: ""
                }

                Text {
                    text: ""
                }
            }

            // Text {
            //     text: model.name + '(' + model.roomid + ')' + ': ' + model.numMembers
            // }
            // Text {
            //     text: model.topic
            // }
        }
    }

    ListView {
        id: roomDirView
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        model: RoomDirectoryModel {}
        delegate: roomDirDelegateV2
    }   

    // function joinRoom(index) {
    //     // body...
    // }
    
    // function previewRoon(index) {
    //     // body...
    // }
}
