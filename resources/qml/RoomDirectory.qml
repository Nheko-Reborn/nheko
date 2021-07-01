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
    minimumWidth: 650
    minimumHeight: 420
    palette: Nheko.colors
    color: Nheko.colors.window
    modality: Qt.WindowModal
    flags: Qt.Dialog
    title: qsTr("Explore Public Rooms")

    ListView {
        id: roomDirView
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        model: RoomDirectoryModel {}
        delegate: Rectangle {
            id: roomDirDelegate

            property color background: Nheko.colors.window
            property color importantText: Nheko.colors.text
            property color unimportantText: Nheko.colors.buttonText
            property color bubbleBackground: Nheko.colors.highlight
            property color bubbleText: Nheko.colors.highlightedText
            property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 2.5)

            color: background
            
            height: avatarSize + 2 * Nheko.paddingMedium
            width: ListView.view.width

            RowLayout {

                spacing: Nheko.paddingMedium
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium

                Avatar {
                    id: roomAvatar

                    Layout.alignment: Qt.AlignVCenter
                    width: avatarSize
                    height: avatarSize
                    url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                    displayName: model.name
                }

                ColumnLayout {
                    id: textContent

                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                    width: parent.width - avatar.width
                    Layout.preferredWidth: parent.width - avatar.width
                    spacing: Nheko.paddingSmall

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        ElidedLabel {
                            Layout.alignment: Qt.AlignBottom
                            color: roomDirDelegate.importantText
                            elideWidth: textContent.width * 0.5 - Nheko.paddingMedium
                            fullText: model.name
                        }
                    }

                    RowLayout {
                        id: roomDescriptionRow
                        Layout.fillWidth: true
                        Layout.preferredWidth: parent.width
                        spacing: Nheko.paddingSmall
                        Layout.alignment: Qt.AlignLeft

                        Label {
                            color: roomDirDelegate.unimportantText
                            font.weight: Font.Thin
                            font.pixelSize: fontMetrics.font.pixelSize * 0.9
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            Layout.fillWidth: true
                            text: model.topic
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            color: roomDirDelegate.unimportantText
                            Layout.fillWidth: false
                            font.weight: Font.Thin
                            font.pixelSize: fontMetrics.font.pixelSize * 0.9
                            text: model.numMembers.toString()
                        }
                    }

                }
            }
        } 
    }
}
