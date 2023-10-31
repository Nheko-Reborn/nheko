// SPDX-FileCopyrightText: Nheko Contributors
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

    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 380
    minimumHeight: 380
    minimumWidth: headerTitle.width + 2 * Nheko.paddingMedium
    width: 340

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok

        onAccepted: readReceiptsRoot.close()
    }

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

            Layout.alignment: Qt.AlignCenter
            color: palette.text
            font.pointSize: fontMetrics.font.pointSize * 1.5
            text: qsTr("Read receipts")
        }
        ScrollView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 200
            ScrollBar.horizontal.visible: false
            padding: Nheko.paddingMedium

            ListView {
                id: readReceiptsList

                boundsBehavior: Flickable.StopAtBounds
                clip: true
                model: readReceipts

                delegate: ItemDelegate {
                    id: del

                    ToolTip.text: model.mxid
                    ToolTip.visible: hovered
                    height: receiptLayout.implicitHeight + Nheko.paddingSmall * 2
                    hoverEnabled: true
                    padding: Nheko.paddingMedium
                    width: ListView.view.width

                    background: Rectangle {
                        color: del.hovered ? palette.dark : readReceiptsRoot.color
                    }

                    onClicked: room.openUserProfile(model.mxid)

                    RowLayout {
                        id: receiptLayout

                        anchors.fill: parent
                        anchors.margins: Nheko.paddingSmall
                        spacing: Nheko.paddingMedium

                        Avatar {
                            id: avatar

                            Layout.preferredHeight: Nheko.avatarSize
                            Layout.preferredWidth: Nheko.avatarSize
                            displayName: model.displayName
                            enabled: false
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            userid: model.mxid
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Nheko.paddingSmall

                            ElidedLabel {
                                Layout.fillWidth: true
                                color: TimelineManager.userColor(model ? model.mxid : "", palette.window)
                                elideWidth: del.width - Nheko.paddingMedium - avatar.width
                                font.pointSize: fontMetrics.font.pointSize
                                fullText: model.displayName
                            }
                            ElidedLabel {
                                Layout.fillWidth: true
                                color: palette.buttonText
                                elideWidth: del.width - Nheko.paddingMedium - avatar.width
                                font.pointSize: fontMetrics.font.pointSize * 0.9
                                fullText: model.timestamp
                            }
                        }
                    }
                    NhekoCursorShape {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }
        }
    }
}
