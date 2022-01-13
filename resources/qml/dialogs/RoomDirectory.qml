// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../ui"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import im.nheko 1.0

ApplicationWindow {
    id: roomDirectoryWindow

    property RoomDirectoryModel publicRooms

    visible: true
    minimumWidth: 650
    minimumHeight: 420
    palette: Nheko.colors
    color: Nheko.colors.window
    modality: Qt.WindowModal
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
            enabled: !Settings.mobileMode
        }

        delegate: Rectangle {
            id: roomDirDelegate

            property color background: Nheko.colors.window
            property color importantText: Nheko.colors.text
            property color unimportantText: Nheko.colors.buttonText
            property int avatarSize: fontMetrics.lineSpacing * 4

            color: background
            height: avatarSize + Nheko.paddingLarge
            width: ListView.view.width

            RowLayout {
                spacing: Nheko.paddingMedium
                anchors.fill: parent
                anchors.margins: Nheko.paddingLarge
                implicitHeight: textContent.height

                Avatar {
                    id: roomAvatar

                    Layout.alignment: Qt.AlignVCenter
                    width: avatarSize
                    height: avatarSize
                    url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                    roomid: model.roomid
                    displayName: model.name
                }

                ColumnLayout {
                    id: textContent

                    Layout.alignment: Qt.AlignLeft
                    width: parent.width - avatar.width
                    Layout.preferredWidth: parent.width - avatar.width
                    spacing: Nheko.paddingSmall

                    ElidedLabel {
                        Layout.alignment: Qt.AlignBottom
                        color: roomDirDelegate.importantText
                        elideWidth: textContent.width - numMembersRectangle.width - buttonRectangle.width
                        font.pixelSize: fontMetrics.font.pixelSize * 1.1
                        fullText: model.name
                    }

                    RowLayout {
                        id: roomDescriptionRow

                        Layout.preferredWidth: parent.width
                        spacing: Nheko.paddingSmall
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        Layout.preferredHeight: fontMetrics.lineSpacing * 4

                        Label {
                            id: roomTopic

                            color: roomDirDelegate.unimportantText
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

                            Layout.margins: Nheko.paddingSmall
                            width: roomCount.width

                            Label {
                                id: roomCount

                                color: roomDirDelegate.unimportantText
                                anchors.centerIn: parent
                                font.pixelSize: fontMetrics.font.pixelSize
                                text: model.numMembers.toString()
                            }

                        }

                        Item {
                            id: buttonRectangle

                            Layout.margins: Nheko.paddingSmall
                            width: joinRoomButton.width

                            Button {
                                id: joinRoomButton

                                visible: model.canJoin
                                anchors.centerIn: parent
                                text: "Join"
                                onClicked: publicRooms.joinRoom(model.index)
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
                foreground: Nheko.colors.mid
            }

        }

    }

    publicRooms: RoomDirectoryModel {
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
            padding: Nheko.paddingMedium
            color: Nheko.colors.text
            placeholderText: qsTr("Search for public rooms")
            onTextChanged: searchTimer.restart()
        }

        MatrixTextField {
            id: chooseServer

            Layout.minimumWidth: 0.3 * header.width
            Layout.maximumWidth: 0.3 * header.width
            padding: Nheko.paddingMedium
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
