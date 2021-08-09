// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import im.nheko 1.0

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
    flags: Qt.Dialog | Qt.WindowCloseButtonHint
    title: qsTr("Explore Public Rooms")

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: roomDirectoryWindow.close()
    }

    header: RowLayout {
        id: searchBarLayout
        spacing: Nheko.paddingMedium
        width: parent.width      

        implicitHeight: roomSearch.height

        MatrixTextField {
            id: roomSearch

            Layout.fillWidth: true

            font.pixelSize: fontMetrics.font.pixelSize
            padding: Nheko.paddingMedium
            color: Nheko.colors.text
            placeholderText: qsTr("Search for public rooms")
            onTextChanged: searchTimer.restart()
        }

        Timer {
            id: searchTimer

            interval: 350
            onTriggered: roomDirView.model.setSearchTerm(roomSearch.text)
        }
    }

    ListView {
        id: roomDirView
        anchors.fill: parent
        model: RoomDirectoryModel {
            id: roomDir
        }
        delegate: Rectangle {
            id: roomDirDelegate

            property color background: Nheko.colors.window
            property color importantText: Nheko.colors.text
            property color unimportantText: Nheko.colors.buttonText
            property int avatarSize: fontMetrics.lineSpacing * 4

            color: background
            
            height: avatarSize + 2.5 * Nheko.paddingMedium
            width: ListView.view.width

            RowLayout {

                spacing: Nheko.paddingMedium
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium
                implicitHeight: textContent.height

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
                    Layout.preferredHeight: roomNameRow.height + roomDescriptionRow.height
                    spacing: Nheko.paddingSmall

                    RowLayout {
                        id: roomNameRow
                        Layout.fillWidth: true
                        spacing: 0

                        ElidedLabel {
                            Layout.alignment: Qt.AlignBottom
                            color: roomDirDelegate.importantText
                            elideWidth: textContent.width * 0.5 - Nheko.paddingMedium
                            font.pixelSize: fontMetrics.font.pixelSize * 1.1
                            fullText: model.name
                        }
                    }

                    RowLayout {
                        id: roomDescriptionRow
                        Layout.fillWidth: true
                        Layout.preferredWidth: parent.width
                        spacing: Nheko.paddingSmall
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
			Layout.preferredHeight: fontMetrics.lineSpacing * 4

                        Label {
                            id: roomTopic
                            color: roomDirDelegate.unimportantText
                            font.weight: Font.Thin
			    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            font.pixelSize: fontMetrics.font.pixelSize
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            Layout.fillWidth: true
                            text: model.topic
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                        }
			Item {
			  id: numMembersRectangle
			  Layout.fillWidth: false
			  Layout.margins: Nheko.paddingSmall
                          width: roomCount.width

                        Label {
                            id: roomCount
                            color: roomDirDelegate.unimportantText
			    anchors.centerIn: parent
                            Layout.fillWidth: false
                            font.weight: Font.Thin
                            font.pixelSize: fontMetrics.font.pixelSize
                            text: model.numMembers.toString()
                        }
			}

			Item {
				id: buttonRectangle
				Layout.fillWidth: false
				Layout.margins: Nheko.paddingSmall	
                        	width: joinRoomButton.width
				Button {
                            		id: joinRoomButton
			    		visible: roomDir.canJoinRoom(model.roomid)
					anchors.centerIn: parent 
                            		width: Math.ceil(0.1 * roomDirectoryWindow.width)
					text: "Join"
                            		onClicked: roomDir.joinRoom(model.index)
                        	}		
			}
                    }
                }
            }
        } 
    }
}
