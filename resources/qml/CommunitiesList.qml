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

            function show(id_, tags_) {
                tagId = id_;
                open();
            }

            Platform.MenuItem {
                text: qsTr("Hide rooms with this tag or from this space by default.")
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
            required property string avatarUrl
            required property string displayName
            required property string tooltip
            required property bool collapsed
            required property bool collapsible
            required property bool hidden
            required property int depth
            required property string id
            required property int unreadMessages
            required property bool hasLoudNotification

            height: avatarSize + 2 * Nheko.paddingMedium
            width: ListView.view.width
            state: "normal"
            ToolTip.visible: hovered && collapsed
            ToolTip.text: communityItem.tooltip
            ToolTip.delay: Nheko.tooltipDelay
            onClicked: Communities.setCurrentTagId(communityItem.id)
            onPressAndHold: communityContextMenu.show(communityItem.id)
            states: [
                State {
                    name: "highlight"
                    when: (communityItem.hovered || communityItem.hidden) && !(Communities.currentTagId === communityItem.id)

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
                    when: Communities.currentTagId == communityItem.id

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
                    onSingleTapped: communityContextMenu.show(communityItem.id)
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                }

            }

            RowLayout {
                id: r
                spacing: Nheko.paddingMedium
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium
                anchors.leftMargin: Nheko.paddingMedium + (communitySidebar.collapsed ? 0 : (fontMetrics.lineSpacing * communityItem.depth))

                ImageButton {
                    visible: !communitySidebar.collapsed && communityItem.collapsible
                    Layout.preferredHeight: fontMetrics.lineSpacing
                    Layout.preferredWidth: fontMetrics.lineSpacing
                    Layout.alignment: Qt.AlignVCenter
                    height: fontMetrics.lineSpacing
                    width: fontMetrics.lineSpacing
                    image: communityItem.collapsed ? ":/icons/icons/ui/collapsed.svg" : ":/icons/icons/ui/expanded.svg"
                    ToolTip.visible: hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: communityItem.collapsed ? qsTr("Expand") : qsTr("Collapse")
                    hoverEnabled: true

                    onClicked: communityItem.collapsed = !communityItem.collapsed
                }

                Item {
                    Layout.preferredWidth: fontMetrics.lineSpacing
                    visible: !communitySidebar.collapsed && !communityItem.collapsible && Communities.containsSubspaces
                }

                Avatar {
                    id: avatar

                    enabled: false
                    Layout.alignment: Qt.AlignVCenter
                    height: avatarSize
                    width: avatarSize
                    url: {
                        if (communityItem.avatarUrl.startsWith("mxc://"))
                            return communityItem.avatarUrl.replace("mxc://", "image://MxcImage/");
                        else
                            return "image://colorimage/" + communityItem.avatarUrl + "?" + communityItem.unimportantText;
                    }
                    roomid: communityItem.id
                    displayName: communityItem.displayName
                    color: communityItem.backgroundColor

                    NotificationBubble {
                        notificationCount: communityItem.unreadMessages
                        hasLoudNotification: communityItem.hasLoudNotification
                        bubbleBackgroundColor: communityItem.bubbleBackground
                        bubbleTextColor: communityItem.bubbleText
                        font.pixelSize: fontMetrics.font.pixelSize * 0.6
                        mayBeVisible: communitySidebar.collapsed
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
                    fullText: communityItem.displayName
                    textFormat: Text.PlainText
                }

                Item {
                    Layout.fillWidth: true
                }

                NotificationBubble {
                    notificationCount: communityItem.unreadMessages
                    hasLoudNotification: communityItem.hasLoudNotification
                    bubbleBackgroundColor: communityItem.bubbleBackground
                    bubbleTextColor: communityItem.bubbleText
                    mayBeVisible: !communitySidebar.collapsed
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
