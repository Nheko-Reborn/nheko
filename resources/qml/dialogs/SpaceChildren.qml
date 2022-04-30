// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0

ApplicationWindow {
    id: spaceChildrenDialog

    property SpaceChildrenModel spaceChildren
    property NonSpaceChildrenModel nonChildren

    minimumWidth: 340
    minimumHeight: 450
    width: 450
    height: 680
    palette: Nheko.colors
    color: Nheko.colors.window
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    title: qsTr("Space children")

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: spaceChildrenDialog.close()
    }

    ScrollHelper {
        flickable: flickable
        anchors.fill: flickable
    }

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Label {
            color: Nheko.colors.text
            horizontalAlignment: Label.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            text: qsTr("Children of %1").arg(spaceChildren.space.roomName)
            wrapMode: Text.Wrap
            font.pointSize: fontMetrics.font.pointSize * 1.5
        }

        ListView {
            id: childrenList

            Layout.fillWidth: true
            Layout.fillHeight: true
            model: spaceChildren
            spacing: Nheko.paddingMedium
            clip: true

            ScrollHelper {
                flickable: parent
                anchors.fill: parent
            }

            delegate: RowLayout {
                id: childDel

                required property string id
                required property string roomName
                required property string avatarUrl
                required property string alias

                spacing: Nheko.paddingMedium
                width: ListView.view.width

                Avatar {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.rightMargin: Nheko.paddingMedium
                    width: Nheko.avatarSize
                    height: Nheko.avatarSize
                    url: childDel.avatarUrl.replace("mxc://", "image://MxcImage/")
                    roomid: childDel.id
                    displayName: childDel.roomName
                }

                ColumnLayout {
                    spacing: Nheko.paddingMedium

                    Label {
                        font.bold: true
                        text: childDel.roomName
                    }

                    Label {
                        text: childDel.alias
                        visible: childDel.alias
                        color: Nheko.inactiveColors.text
                    }
                }

                Item { Layout.fillWidth: true }

                ImageButton {
                    image: ":/icons/icons/ui/delete.svg"
                    onClicked: Nheko.removeRoomFromSpace(childDel.id, spaceChildren.space.roomId)
                }
            }
        }

        Label {
            color: Nheko.colors.text
            horizontalAlignment: Label.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            text: qsTr("Add rooms to %1").arg(spaceChildren.space.roomName)
            wrapMode: Text.Wrap
            font.pointSize: fontMetrics.font.pointSize * 1.5
        }

        ListView {
            id: nonChildrenList

            Layout.fillWidth: true
            Layout.fillHeight: true
            model: nonChildren
            spacing: Nheko.paddingMedium
            clip: true

            ScrollHelper {
                flickable: parent
                anchors.fill: parent
            }

            delegate: RowLayout {
                id: nonChildDel

                required property string id
                required property string roomName
                required property string avatarUrl
                required property string alias

                spacing: Nheko.paddingMedium
                width: ListView.view.width

                Avatar {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.rightMargin: Nheko.paddingMedium
                    width: Nheko.avatarSize
                    height: Nheko.avatarSize
                    url: nonChildDel.avatarUrl.replace("mxc://", "image://MxcImage/")
                    roomid: nonChildDel.id
                    displayName: nonChildDel.roomName
                }

                ColumnLayout {
                    spacing: Nheko.paddingMedium

                    Label {
                        font.bold: true
                        text: nonChildDel.roomName
                    }

                    Label {
                        text: nonChildDel.alias
                        visible: nonChildDel.alias
                        color: Nheko.inactiveColors.text
                    }
                }

                Item { Layout.fillWidth: true }

                ImageButton {
                    image: ":/icons/icons/ui/add-square-button.svg"
                    onClicked: Nheko.addRoomToSpace(nonChildDel.id, spaceChildren.space.roomId)
                }
            }
        }
    }

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok
        onAccepted: close()
    }
}
