// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./components"
import "./dialogs"
import Qt.labs.platform 1.1 as Platform
import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko 1.0

Page {
    id: communitySidebar
    //leftPadding: Nheko.paddingSmall
    //rightPadding: Nheko.paddingSmall
    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 1.6)
    property bool collapsed: false

    ListView {
        id: communitiesList

        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        model: Communities.filtered()

        ScrollHelper {
            flickable: parent
            anchors.fill: parent
            enabled: !Settings.mobileMode
        }

        Platform.Menu {
            id: communityContextMenu

            property string tagId
            property bool hidden
            property bool muted

            function show(id_, hidden_, muted_) {
                tagId = id_;
                hidden = hidden_;
                muted = muted_;
                open();
            }

            Platform.MenuItem {
                text: qsTr("Do not show notification counts for this space or tag.")
                checkable: true
                checked: communityContextMenu.muted
                onTriggered: Communities.toggleTagMute(communityContextMenu.tagId)
            }

            Platform.MenuItem {
                text: qsTr("Hide rooms with this tag or from this space by default.")
                checkable: true
                checked: communityContextMenu.hidden
                onTriggered: Communities.toggleTagId(communityContextMenu.tagId)
            }

        }

        delegate: ItemDelegate {
            id: communityItem

            property color backgroundColor: Nheko.colors.window
            property color importantText: Nheko.colors.text
            property color unimportantText: Nheko.colors.buttonText
            property color bubbleBackground: Nheko.colors.highlight
            property color bubbleText: Nheko.colors.highlightedText
            required property var model

            height: avatarSize + 2 * Nheko.paddingMedium
            width: ListView.view.width
            state: "normal"
            ToolTip.visible: hovered && collapsed
            ToolTip.text: model.tooltip
            ToolTip.delay: Nheko.tooltipDelay
            onClicked: Communities.setCurrentTagId(model.id)
            onPressAndHold: communityContextMenu.show(model.id, model.hidden, model.muted)
            states: [
                State {
                    name: "highlight"
                    when: (communityItem.hovered || model.hidden) && !(Communities.currentTagId === model.id)

                    PropertyChanges {
                        target: communityItem
                        backgroundColor: Nheko.colors.dark
                        importantText: Nheko.colors.brightText
                        unimportantText: Nheko.colors.brightText
                        bubbleBackground: Nheko.colors.highlight
                        bubbleText: Nheko.colors.highlightedText
                    }

                },
                State {
                    name: "selected"
                    when: Communities.currentTagId == model.id

                    PropertyChanges {
                        target: communityItem
                        backgroundColor: Nheko.colors.highlight
                        importantText: Nheko.colors.highlightedText
                        unimportantText: Nheko.colors.highlightedText
                        bubbleBackground: Nheko.colors.highlightedText
                        bubbleText: Nheko.colors.highlight
                    }

                }
            ]

            Item {
                anchors.fill: parent

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onSingleTapped: communityContextMenu.show(model.id, model.hidden, model.muted)
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                }

            }

            RowLayout {
                id: r
                spacing: Nheko.paddingMedium
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium
                anchors.leftMargin: Nheko.paddingMedium + (communitySidebar.collapsed ? 0 : (fontMetrics.lineSpacing * model.depth))

                ImageButton {
                    visible: !communitySidebar.collapsed && model.collapsible
                    Layout.preferredHeight: fontMetrics.lineSpacing
                    Layout.preferredWidth: fontMetrics.lineSpacing
                    Layout.alignment: Qt.AlignVCenter
                    height: fontMetrics.lineSpacing
                    width: fontMetrics.lineSpacing
                    image: model.collapsed ? ":/icons/icons/ui/collapsed.svg" : ":/icons/icons/ui/expanded.svg"
                    ToolTip.visible: hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: model.collapsed ? qsTr("Expand") : qsTr("Collapse")
                    hoverEnabled: true

                    onClicked: model.collapsed = !model.collapsed
                }

                Item {
                    Layout.preferredWidth: fontMetrics.lineSpacing
                    visible: !communitySidebar.collapsed && !model.collapsible && Communities.containsSubspaces
                }

                Avatar {
                    id: avatar

                    enabled: false
                    Layout.alignment: Qt.AlignVCenter
                    height: avatarSize
                    width: avatarSize
                    url: {
                        if (model.avatarUrl.startsWith("mxc://"))
                            return model.avatarUrl.replace("mxc://", "image://MxcImage/");
                        else
                            return "image://colorimage/" + model.avatarUrl + "?" + communityItem.unimportantText;
                    }
                    roomid: model.id
                    displayName: model.displayName
                    color: communityItem.backgroundColor

                    NotificationBubble {
                        notificationCount: model.unreadMessages
                        hasLoudNotification: model.hasLoudNotification
                        bubbleBackgroundColor: communityItem.bubbleBackground
                        bubbleTextColor: communityItem.bubbleText
                        font.pixelSize: fontMetrics.font.pixelSize * 0.6
                        mayBeVisible: communitySidebar.collapsed && !model.muted && Settings.spaceNotifications
                        anchors.right: avatar.right
                        anchors.bottom: avatar.bottom
                        anchors.margins: -Nheko.paddingSmall
                    }

                }

                ElidedLabel {
                    visible: !communitySidebar.collapsed
                    Layout.alignment: Qt.AlignVCenter
                    color: communityItem.importantText
                    Layout.fillWidth: true
                    elideWidth: width
                    fullText: model.displayName
                    textFormat: Text.PlainText
                }

                Item {
                    Layout.fillWidth: true
                }

                NotificationBubble {
                    notificationCount: model.unreadMessages
                    hasLoudNotification: model.hasLoudNotification
                    bubbleBackgroundColor: communityItem.bubbleBackground
                    bubbleTextColor: communityItem.bubbleText
                    mayBeVisible: !communitySidebar.collapsed && !model.muted && Settings.spaceNotifications
                    Layout.alignment: Qt.AlignRight
                    Layout.leftMargin: Nheko.paddingSmall
                }

            }

            background: Rectangle {
                color: communityItem.backgroundColor
            }

        }

    }

    background: Rectangle {
        color: Nheko.theme.sidebarBackground
    }

}
