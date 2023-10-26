// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../ui"
import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import QtQuick.Window 2.15
import im.nheko 1.0

ApplicationWindow {
    id: roomDirectoryWindow

    visible: true
    minimumWidth: 340
    minimumHeight: 340
    height: 420
    width: 650
    color: palette.window
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    title: qsTr("Explore Public Rooms")

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: roomDirectoryWindow.close()
    }

    ListView {
        id: roomDirView

        anchors.fill: parent
        model: publicRooms

        delegate: Rectangle {
            id: roomDirDelegate

            property color background: palette.window
            property color importantText: palette.text
            property color unimportantText: palette.buttonText
            property int avatarSize: fontMetrics.height * 3.2

            color: background
            height: avatarSize + Nheko.paddingLarge
            width: ListView.view.width

            RowLayout {
                spacing: Nheko.paddingMedium
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium
                implicitHeight: textContent.implicitHeight

                Avatar {
                    id: roomAvatar

                    Layout.alignment: Qt.AlignVCenter
                    Layout.rightMargin: Nheko.paddingMedium
                    Layout.preferredWidth: roomDirDelegate.avatarSize
                    Layout.preferredHeight: roomDirDelegate.avatarSize

                    url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                    roomid: model.roomid
                    displayName: model.name
                }

                GridLayout {
                    id: textContent
                    rows: 2
                    columns: 2

                    Layout.alignment: Qt.AlignLeft
                    Layout.preferredWidth: parent.width - roomAvatar.width

                    ElidedLabel {
                        Layout.row: 0
                        Layout.column: 0
                        Layout.fillWidth:true
                        color: roomDirDelegate.importantText
                        elideWidth: width
                        fullText: model.name
                    }

                    Label {
                        id: roomTopic

                        color: roomDirDelegate.unimportantText
                        Layout.row: 1
                        Layout.column: 0
                        font.pointSize: fontMetrics.font.pointSize*0.9
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        Layout.fillWidth: true
                        text: model.topic
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.row: 0
                        Layout.column: 1
                        id: roomCount

                        color: roomDirDelegate.unimportantText
                        font.pointSize: fontMetrics.font.pointSize*0.9
                        text: model.numMembers.toString()
                    }

                    Button {
                        Layout.row: 1
                        Layout.column: 1
                        id: joinRoomButton
                        enabled: model.roomid !== ""
                        text: model.canJoin ? qsTr("Join") : qsTr("Open")
                        onClicked: {
                            if (model.canJoin)
                                publicRooms.joinRoom(model.index);
                            else
                            {
                                Rooms.setCurrentRoom(model.roomid);
                                roomDirectoryWindow.close();
                            }
                        }
                    }

                }

            }

        }

        footer: Item {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            visible: !publicRooms.reachedEndOfPagination && publicRooms.loadingMoreRooms
            // hacky but works
            height: loadingSpinner.height + 2 * Nheko.paddingLarge
            anchors.margins: Nheko.paddingLarge

            Spinner {
                id: loadingSpinner

                anchors.centerIn: parent
                anchors.margins: Nheko.paddingLarge
                running: visible
                foreground: palette.mid
            }

        }

    }

    header: RowLayout {
        id: searchBarLayout

        spacing: Nheko.paddingMedium
        width: parent.width
        implicitHeight: roomSearch.height

        MatrixTextField {
            id: roomSearch

            focus: true
            Layout.fillWidth: true
            selectByMouse: true
            font.pixelSize: fontMetrics.font.pixelSize
            color: palette.text
            placeholderText: qsTr("Search for public rooms")
            onTextChanged: searchTimer.restart()

            Component.onCompleted: forceActiveFocus()
        }

        MatrixTextField {
            id: chooseServer

            Layout.minimumWidth: 0.3 * header.width
            Layout.maximumWidth: 0.3 * header.width
            color: palette.text
            placeholderText: qsTr("Choose custom homeserver")
            onTextChanged: publicRooms.setMatrixServer(text)
        }

        Timer {
            id: searchTimer

            interval: 350
            onTriggered: roomDirView.model.setSearchTerm(roomSearch.text)
        }

    }

    footer: RowLayout {
        spacing: Nheko.paddingMedium
        width: parent.width

        Button {
            text: qsTr("Close")
            onClicked: roomDirectoryWindow.close()
            Layout.alignment: Qt.AlignRight
            Layout.margins: Nheko.paddingMedium
        }
    }

}
