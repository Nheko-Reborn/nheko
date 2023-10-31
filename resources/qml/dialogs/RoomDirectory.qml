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

    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 420
    minimumHeight: 340
    minimumWidth: 340
    modality: Qt.NonModal
    title: qsTr("Explore Public Rooms")
    visible: true
    width: 650

    footer: RowLayout {
        spacing: Nheko.paddingMedium
        width: parent.width

        Button {
            Layout.alignment: Qt.AlignRight
            Layout.margins: Nheko.paddingMedium
            text: qsTr("Close")

            onClicked: roomDirectoryWindow.close()
        }
    }
    header: RowLayout {
        id: searchBarLayout

        implicitHeight: roomSearch.height
        spacing: Nheko.paddingMedium
        width: parent.width

        MatrixTextField {
            id: roomSearch

            Layout.fillWidth: true
            color: palette.text
            focus: true
            font.pixelSize: fontMetrics.font.pixelSize
            placeholderText: qsTr("Search for public rooms")
            selectByMouse: true

            Component.onCompleted: forceActiveFocus()
            onTextChanged: searchTimer.restart()
        }
        MatrixTextField {
            id: chooseServer

            Layout.maximumWidth: 0.3 * header.width
            Layout.minimumWidth: 0.3 * header.width
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

            property int avatarSize: fontMetrics.height * 3.2
            property color background: palette.window
            property color importantText: palette.text
            property color unimportantText: palette.buttonText

            color: background
            height: avatarSize + Nheko.paddingLarge
            width: ListView.view.width

            RowLayout {
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium
                implicitHeight: textContent.implicitHeight
                spacing: Nheko.paddingMedium

                Avatar {
                    id: roomAvatar

                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredHeight: roomDirDelegate.avatarSize
                    Layout.preferredWidth: roomDirDelegate.avatarSize
                    Layout.rightMargin: Nheko.paddingMedium
                    displayName: model.name
                    roomid: model.roomid
                    url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                }
                GridLayout {
                    id: textContent

                    Layout.alignment: Qt.AlignLeft
                    Layout.preferredWidth: parent.width - roomAvatar.width
                    columns: 2
                    rows: 2

                    ElidedLabel {
                        Layout.column: 0
                        Layout.fillWidth: true
                        Layout.row: 0
                        color: roomDirDelegate.importantText
                        elideWidth: width
                        fullText: model.name
                    }
                    Label {
                        id: roomTopic

                        Layout.column: 0
                        Layout.fillWidth: true
                        Layout.row: 1
                        color: roomDirDelegate.unimportantText
                        elide: Text.ElideRight
                        font.pointSize: fontMetrics.font.pointSize * 0.9
                        maximumLineCount: 2
                        text: model.topic
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        id: roomCount

                        Layout.alignment: Qt.AlignHCenter
                        Layout.column: 1
                        Layout.row: 0
                        color: roomDirDelegate.unimportantText
                        font.pointSize: fontMetrics.font.pointSize * 0.9
                        text: model.numMembers.toString()
                    }
                    Button {
                        id: joinRoomButton

                        Layout.column: 1
                        Layout.row: 1
                        enabled: model.roomid !== ""
                        text: model.canJoin ? qsTr("Join") : qsTr("Open")

                        onClicked: {
                            if (model.canJoin)
                                publicRooms.joinRoom(model.index);
                            else {
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
            anchors.margins: Nheko.paddingLarge
            // hacky but works
            height: loadingSpinner.height + 2 * Nheko.paddingLarge
            visible: !publicRooms.reachedEndOfPagination && publicRooms.loadingMoreRooms
            width: parent.width

            Spinner {
                id: loadingSpinner

                anchors.centerIn: parent
                anchors.margins: Nheko.paddingLarge
                foreground: palette.mid
                running: visible
            }
        }
    }
}
