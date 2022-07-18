// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
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
    palette: Nheko.colors
    color: Nheko.colors.window
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

        ScrollHelper {
            flickable: parent
            anchors.fill: parent
        }

        delegate: Rectangle {
            id: roomDirDelegate

            property color background: Nheko.colors.window
            property color importantText: Nheko.colors.text
            property color unimportantText: Nheko.colors.buttonText
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
                    width: avatarSize
                    height: avatarSize
                    url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                    roomid: model.roomid
                    displayName: model.name
                }

                GridLayout {
                    id: textContent
                    rows: 2
                    columns: 2

                    Layout.alignment: Qt.AlignLeft
                    width: parent.width - avatar.width
                    Layout.preferredWidth: parent.width - avatar.width

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
                        enabled: model.canJoin
                        text: "Join"
                        onClicked: publicRooms.joinRoom(model.index)
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
                foreground: Nheko.colors.mid
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
            color: Nheko.colors.text
            placeholderText: qsTr("Search for public rooms")
            onTextChanged: searchTimer.restart()

            Component.onCompleted: forceActiveFocus()
        }

        MatrixTextField {
            id: chooseServer

            Layout.minimumWidth: 0.3 * header.width
            Layout.maximumWidth: 0.3 * header.width
            color: Nheko.colors.text
            placeholderText: qsTr("Choose custom homeserver")
            onTextChanged: publicRooms.setMatrixServer(text)
        }

        Timer {
            id: searchTimer

            interval: 350
            onTriggered: roomDirView.model.setSearchTerm(roomSearch.text)
        }

    }

}
