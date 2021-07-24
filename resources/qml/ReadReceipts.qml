// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

ApplicationWindow {
    id: readReceiptsRoot

    property ReadReceiptsModel readReceipts

    x: MainWindow.x + (MainWindow.width / 2) - (width / 2)
    y: MainWindow.y + (MainWindow.height / 2) - (height / 2)
    height: 380
    width: 340
    minimumHeight: 380
    minimumWidth: headerTitle.width + 2 * Nheko.paddingMedium
    palette: Nheko.colors
    color: Nheko.colors.window

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Label {
            id: headerTitle

            Layout.alignment: Qt.AlignCenter
            text: qsTr("Read receipts")
            font.pointSize: fontMetrics.font.pointSize * 1.5
        }

        ScrollView {
            palette: Nheko.colors
            padding: Nheko.paddingMedium
            ScrollBar.horizontal.visible: false
            Layout.fillHeight: true
            Layout.minimumHeight: 200
            Layout.fillWidth: true

            ListView {
                id: readReceiptsList

                clip: true
                spacing: Nheko.paddingMedium
                boundsBehavior: Flickable.StopAtBounds
                model: readReceipts

                delegate: RowLayout {
                    spacing: Nheko.paddingMedium

                    Avatar {
                        width: Nheko.avatarSize
                        height: Nheko.avatarSize
                        userid: model.mxid
                        url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                        displayName: model.displayName
                        onClicked: Rooms.currentRoom.openUserProfile(model.mxid)
                        ToolTip.visible: avatarHover.hovered
                        ToolTip.text: model.mxid

                        HoverHandler {
                            id: avatarHover
                        }

                    }

                    ColumnLayout {
                        spacing: Nheko.paddingSmall

                        Label {
                            text: model.displayName
                            color: TimelineManager.userColor(model ? model.mxid : "", Nheko.colors.window)
                            font.pointSize: fontMetrics.font.pointSize
                            ToolTip.visible: displayNameHover.hovered
                            ToolTip.text: model.mxid

                            TapHandler {
                                onSingleTapped: chat.model.openUserProfile(userId)
                            }

                            CursorShape {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                            }

                            HoverHandler {
                                id: displayNameHover
                            }

                        }

                        Label {
                            text: model.timestamp
                            color: Nheko.colors.buttonText
                            font.pointSize: fontMetrics.font.pointSize * 0.9
                        }

                        Item {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                        }

                    }

                }

            }

        }

    }

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok
        onAccepted: readReceiptsRoot.close()

    }

}
