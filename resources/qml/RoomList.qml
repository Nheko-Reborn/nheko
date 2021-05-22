// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import Qt.labs.platform 1.1 as Platform
import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.3
import im.nheko 1.0

Page {
    ListView {
        id: roomlist

        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        model: Rooms

        ScrollHelper {
            flickable: parent
            anchors.fill: parent
            enabled: !Settings.mobileMode
        }

        Connections {
            onActiveTimelineChanged: {
                roomlist.positionViewAtIndex(Rooms.roomidToIndex(TimelineManager.timeline.roomId()), ListView.Contain);
                console.log("Test" + TimelineManager.timeline.roomId() + " " + Rooms.roomidToIndex(TimelineManager.timeline.roomId));
            }
            target: TimelineManager
        }

        delegate: Rectangle {
            id: roomItem

            property color background: Nheko.colors.window
            property color importantText: Nheko.colors.text
            property color unimportantText: Nheko.colors.buttonText
            property color bubbleBackground: Nheko.colors.highlight
            property color bubbleText: Nheko.colors.highlightedText

            color: background
            height: Math.ceil(fontMetrics.lineSpacing * 2.3 + Nheko.paddingMedium * 2)
            width: ListView.view.width
            state: "normal"
            states: [
                State {
                    name: "highlight"
                    when: hovered.hovered && !(TimelineManager.timeline && model.roomId == TimelineManager.timeline.roomId())

                    PropertyChanges {
                        target: roomItem
                        background: Nheko.colors.dark
                        importantText: Nheko.colors.brightText
                        unimportantText: Nheko.colors.brightText
                        bubbleBackground: Nheko.colors.highlight
                        bubbleText: Nheko.colors.highlightedText
                    }

                },
                State {
                    name: "selected"
                    when: TimelineManager.timeline && model.roomId == TimelineManager.timeline.roomId()

                    PropertyChanges {
                        target: roomItem
                        background: Nheko.colors.highlight
                        importantText: Nheko.colors.highlightedText
                        unimportantText: Nheko.colors.highlightedText
                        bubbleBackground: Nheko.colors.highlightedText
                        bubbleText: Nheko.colors.highlight
                    }

                }
            ]

            HoverHandler {
                id: hovered
            }

            TapHandler {
                onSingleTapped: TimelineManager.setHistoryView(model.roomId)
            }

            RowLayout {
                spacing: Nheko.paddingMedium
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium

                Avatar {
                    // In the future we could show an online indicator by setting the userid for the avatar
                    //userid: Nheko.currentUser.userid

                    id: avatar

                    Layout.alignment: Qt.AlignVCenter
                    height: Math.ceil(fontMetrics.lineSpacing * 2.3)
                    width: Math.ceil(fontMetrics.lineSpacing * 2.3)
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
                    spacing: Nheko.paddingSmall

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        ElidedLabel {
                            Layout.alignment: Qt.AlignBottom
                            color: roomItem.importantText
                            elideWidth: textContent.width - timestamp.width - Nheko.paddingMedium
                            fullText: model.roomName
                            textFormat: Text.RichText
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            id: timestamp

                            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                            font.pixelSize: fontMetrics.font.pixelSize * 0.9
                            color: roomItem.unimportantText
                            text: model.time
                        }

                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        ElidedLabel {
                            color: roomItem.unimportantText
                            font.weight: Font.Thin
                            font.pixelSize: fontMetrics.font.pixelSize * 0.9
                            elideWidth: textContent.width - (notificationBubble.visible ? notificationBubble.width : 0) - Nheko.paddingSmall
                            fullText: model.lastMessage
                            textFormat: Text.RichText
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            id: notificationBubble

                            visible: model.notificationCount > 0
                            Layout.alignment: Qt.AlignRight
                            height: fontMetrics.averageCharacterWidth * 3
                            width: height
                            radius: height / 2
                            color: model.hasLoudNotification ? Nheko.theme.red : roomItem.bubbleBackground

                            Label {
                                anchors.centerIn: parent
                                width: parent.width * 0.8
                                height: parent.height * 0.8
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                fontSizeMode: Text.Fit
                                font.bold: true
                                font.pixelSize: fontMetrics.font.pixelSize * 0.8
                                color: model.hasLoudNotification ? "white" : roomItem.bubbleText
                                text: model.notificationCount > 99 ? "99+" : model.notificationCount
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
            id: userInfoPanel

            function openUserProfile() {
                Nheko.updateUserProfile();
                var userProfile = userProfileComponent.createObject(timelineRoot, {
                    "profile": Nheko.currentUser
                });
                userProfile.show();
            }

            color: Nheko.colors.window
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            Layout.preferredHeight: userInfoGrid.implicitHeight + 2 * Nheko.paddingMedium
            Layout.minimumHeight: 40

            ApplicationWindow {
                id: statusDialog

                modality: Qt.NonModal
                flags: Qt.Dialog
                title: qsTr("Status Message")
                width: 350
                height: fontMetrics.lineSpacing * 7

                ColumnLayout {
                    anchors.margins: Nheko.paddingLarge
                    anchors.fill: parent

                    Label {
                        color: Nheko.colors.text
                        text: qsTr("Enter your status message:")
                    }

                    MatrixTextField {
                        id: statusInput

                        Layout.fillWidth: true
                    }

                }

                footer: DialogButtonBox {
                    standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
                    onAccepted: {
                        Nheko.setStatusMessage(statusInput.text);
                        statusDialog.close();
                    }
                    onRejected: {
                        statusDialog.close();
                    }
                }

            }

            Platform.Menu {
                id: userInfoMenu

                Platform.MenuItem {
                    text: qsTr("Profile settings")
                    onTriggered: userInfoPanel.openUserProfile()
                }

                Platform.MenuItem {
                    text: qsTr("Set status message")
                    onTriggered: statusDialog.show()
                }

            }

            TapHandler {
                acceptedButtons: Qt.LeftButton
                onSingleTapped: userInfoPanel.openUserProfile()
                onLongPressed: userInfoMenu.open()
                gesturePolicy: TapHandler.ReleaseWithinBounds
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                onSingleTapped: userInfoMenu.open()
                gesturePolicy: TapHandler.ReleaseWithinBounds
            }

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
