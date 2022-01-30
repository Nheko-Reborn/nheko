// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

ApplicationWindow {
    id: readReceiptsRoot

    property ReadReceiptsProxy readReceipts
    property Room room

    height: 380
    width: 340
    minimumHeight: 380
    minimumWidth: headerTitle.width + 2 * Nheko.paddingMedium
    palette: Nheko.colors
    color: Nheko.colors.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: readReceiptsRoot.close()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Label {
            id: headerTitle

            color: Nheko.colors.text
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
                boundsBehavior: Flickable.StopAtBounds
                model: readReceipts

                delegate: ItemDelegate {
                    id: del

                    onClicked: room.openUserProfile(model.mxid)
                    padding: Nheko.paddingMedium
                    width: ListView.view.width
                    height: receiptLayout.implicitHeight + Nheko.paddingSmall * 2
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: model.mxid
                    background: Rectangle {
                        color: del.hovered ? Nheko.colors.dark : readReceiptsRoot.color
                    }

                    RowLayout {
                        id: receiptLayout

                        spacing: Nheko.paddingMedium
                        anchors.fill: parent
                        anchors.margins: Nheko.paddingSmall

                        Avatar {
                            width: Nheko.avatarSize
                            height: Nheko.avatarSize
                            userid: model.mxid
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            displayName: model.displayName
                            enabled: false
                        }

                        ColumnLayout {
                            spacing: Nheko.paddingSmall

                            Label {
                                text: model.displayName
                                color: TimelineManager.userColor(model ? model.mxid : "", Nheko.colors.window)
                                font.pointSize: fontMetrics.font.pointSize
                            }

                            Label {
                                text: model.timestamp
                                color: Nheko.colors.buttonText
                                font.pointSize: fontMetrics.font.pointSize * 0.9
                            }

                        }

                        Item {
                            Layout.fillWidth: true
                        }

                    }

                    CursorShape {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
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
