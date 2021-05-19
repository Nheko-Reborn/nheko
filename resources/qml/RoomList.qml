// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.3
import im.nheko 1.0

Page {
    ListView {
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        model: Rooms

        ScrollHelper {
            flickable: parent
            anchors.fill: parent
            enabled: !Settings.mobileMode
        }

        delegate: Rectangle {
            color: Nheko.colors.window
            height: fontMetrics.lineSpacing * 2.5 + Nheko.paddingMedium * 2
            width: ListView.view.width

            RowLayout {
                //id: userInfoGrid

                spacing: Nheko.paddingMedium
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium

                Avatar {
                    //userid: Nheko.currentUser.userid

                    id: avatar

                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: fontMetrics.lineSpacing * 2.5
                    Layout.preferredHeight: fontMetrics.lineSpacing * 2.5
                    url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                    displayName: model.roomName
                }

                ColumnLayout {
                    id: textContent

                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                    Layout.minimumWidth: 100
                    width: parent.width - avatar.width
                    Layout.preferredWidth: parent.width - avatar.width
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        ElidedLabel {
                            Layout.alignment: Qt.AlignBottom
                            color: Nheko.colors.text
                            elideWidth: textContent.width - timestamp.width - Nheko.paddingMedium
                            fullText: model.roomName + ": " + model.notificationCount
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            id: timestamp

                            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                            font.pixelSize: fontMetrics.font.pixelSize * 0.9
                            color: Nheko.colors.buttonText
                            text: "14:32"
                        }

                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        ElidedLabel {
                            color: Nheko.colors.buttonText
                            font.weight: Font.Thin
                            font.pixelSize: fontMetrics.font.pixelSize * 0.9
                            elideWidth: textContent.width - notificationBubble.width
                            fullText: model.lastMessage
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            id: notificationBubble

                            Layout.alignment: Qt.AlignRight
                            height: fontMetrics.font.pixelSize * 1.3
                            width: height
                            radius: height / 2
                            color: Nheko.colors.highlight

                            Label {
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                fontSizeMode: Text.Fit
                                color: Nheko.colors.highlightedText
                                text: model.notificationCount
                            }

                        }

                    }

                }

            }

            Rectangle {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                height: parent.height - Nheko.paddingSmall * 2
                width: 3
                color: Nheko.colors.highlight
                visible: model.hasUnreadMessages
            }

        }

    }

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
                    Layout.preferredWidth: fontMetrics.lineSpacing * 2
                    Layout.preferredHeight: fontMetrics.lineSpacing * 2
                    url: Nheko.currentUser.avatarUrl.replace("mxc://", "image://MxcImage/")
                    displayName: Nheko.currentUser.displayName
                    userid: Nheko.currentUser.userid
                }

                ColumnLayout {
                    id: col

                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                    width: parent.width - avatar.width - logoutButton.width - Nheko.paddingMedium * 2
                    Layout.preferredWidth: parent.width - avatar.width - logoutButton.width - Nheko.paddingMedium * 2
                    spacing: 0

                    ElidedLabel {
                        Layout.alignment: Qt.AlignBottom
                        font.pointSize: fontMetrics.font.pointSize * 1.1
                        font.weight: Font.DemiBold
                        fullText: Nheko.currentUser.displayName
                        elideWidth: col.width
                    }

                    ElidedLabel {
                        Layout.alignment: Qt.AlignTop
                        color: Nheko.colors.buttonText
                        font.weight: Font.Thin
                        font.pointSize: fontMetrics.font.pointSize * 0.9
                        elideWidth: col.width
                        fullText: Nheko.currentUser.userid
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
