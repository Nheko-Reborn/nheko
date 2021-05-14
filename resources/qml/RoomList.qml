// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.3
import im.nheko 1.0

Page {

    background: Rectangle {
        color: Nheko.theme.sidebarBackground
    }

    header: ColumnLayout {
        spacing: 0

        Rectangle {
            color: Nheko.colors.window
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            Layout.preferredHeight: userInfoGrid.implicitHeight + 2 * Nheko.paddingMedium
            Layout.minimumHeight: 40

            RowLayout {
                id: userInfoGrid

                spacing: Nheko.paddingMedium
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium

                Avatar {
                    id: avatar

                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: Nheko.avatarSize
                    Layout.preferredHeight: Nheko.avatarSize
                    url: Nheko.currentUser.avatarUrl.replace("mxc://", "image://MxcImage/")
                    displayName: Nheko.currentUser.displayName
                    userid: Nheko.currentUser.userid
                }

                ColumnLayout {
                    id: col

                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                    Layout.minimumWidth: 100
                    width: parent.width - avatar.width - logoutButton.width
                    Layout.preferredWidth: parent.width - avatar.width - logoutButton.width
                    spacing: 0

                    Label {
                        Layout.alignment: Qt.AlignBottom
                        color: Nheko.colors.text
                        font.pointSize: fontMetrics.font.pointSize * 1.1
                        font.weight: Font.DemiBold
                        text: userNameText.elidedText
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        textFormat: Text.PlainText

                        TextMetrics {
                            id: userNameText

                            font.pointSize: fontMetrics.font.pointSize * 1.1
                            elide: Text.ElideRight
                            elideWidth: col.width
                            text: Nheko.currentUser.displayName
                        }

                    }

                    Label {
                        Layout.alignment: Qt.AlignTop
                        color: Nheko.colors.buttonText
                        font.weight: Font.Thin
                        text: userIdText.elidedText
                        maximumLineCount: 1
                        textFormat: Text.PlainText
                        font.pointSize: fontMetrics.font.pointSize * 0.9

                        TextMetrics {
                            id: userIdText

                            font.pointSize: fontMetrics.font.pointSize * 0.9
                            elide: Text.ElideRight
                            elideWidth: col.width
                            text: Nheko.currentUser.userid
                        }

                    }

                }

                Item {
                }

                ImageButton {
                    id: logoutButton

                    Layout.alignment: Qt.AlignVCenter
                    image: ":/icons/icons/ui/power-button-off.png"
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Logout")
                }

            }

        }

        Rectangle {
            color: Nheko.theme.separator
            height: 2
            Layout.fillWidth: true
        }

    }

    footer: ColumnLayout {
        spacing: 0

        Rectangle {
            color: Nheko.theme.separator
            height: 1
            Layout.fillWidth: true
        }

        Rectangle {
            color: Nheko.colors.window
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            Layout.preferredHeight: buttonRow.implicitHeight
            Layout.minimumHeight: 40

            RowLayout {
                id: buttonRow

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Nheko.paddingMedium

                ImageButton {
                    Layout.alignment: Qt.AlignBottom | Qt.AlignLeft
                    hoverEnabled: true
                    width: 22
                    height: 22
                    image: ":/icons/icons/ui/plus-black-symbol.png"
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Start a new chat")
                    Layout.margins: Nheko.paddingMedium
                }

                ImageButton {
                    Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
                    hoverEnabled: true
                    width: 22
                    height: 22
                    image: ":/icons/icons/ui/speech-bubbles-comment-option.png"
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Room directory")
                    Layout.margins: Nheko.paddingMedium
                }

                ImageButton {
                    Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                    hoverEnabled: true
                    width: 22
                    height: 22
                    image: ":/icons/icons/ui/settings.png"
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("User settings")
                    Layout.margins: Nheko.paddingMedium
                }

            }

        }

    }

}
