// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import "../ui"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import im.nheko

ApplicationWindow {
    id: roomDirectoryWindow

    property RoomDirectoryModel publicRooms: RoomDirectoryModel {
    }

    color: timelineRoot.palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 420
    minimumHeight: 340
    minimumWidth: 340
    modality: Qt.WindowModal
    palette: timelineRoot.palette
    title: qsTr("Explore Public Rooms")
    visible: true
    width: 650

    header: RowLayout {
        id: searchBarLayout
        implicitHeight: roomSearch.height
        spacing: Nheko.paddingMedium
        width: parent.width

        MatrixTextField {
            id: roomSearch
            Layout.fillWidth: true
            color: timelineRoot.palette.text
            focus: true
            font.pixelSize: fontMetrics.font.pixelSize
            placeholderText: qsTr("Search for public rooms")
            selectByMouse: true

            onTextChanged: searchTimer.restart()
        }
        MatrixTextField {
            id: chooseServer
            Layout.maximumWidth: 0.3 * header.width
            Layout.minimumWidth: 0.3 * header.width
            color: timelineRoot.palette.text
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
            property color background: timelineRoot.palette.window
            property color importantText: timelineRoot.palette.text
            property color unimportantText: timelineRoot.palette.placeholderText

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
                    Layout.rightMargin: Nheko.paddingMedium
                    displayName: model.name
                    height: avatarSize
                    roomid: model.roomid
                    url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                    width: avatarSize
                }
                GridLayout {
                    id: textContent
                    Layout.alignment: Qt.AlignLeft
                    Layout.preferredWidth: parent.width - avatar.width
                    columns: 2
                    rows: 2
                    width: parent.width - avatar.width

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
                        enabled: model.canJoin
                        text: "Join"

                        onClicked: publicRooms.joinRoom(model.index)
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
                foreground: timelineRoot.palette.mid
                running: visible
            }
        }
    }
}
