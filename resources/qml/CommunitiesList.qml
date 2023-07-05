// SPDX-FileCopyrightText: Nheko Contributors
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

    background: Rectangle {
        color: Nheko.theme.sidebarBackground
    }

    // HACK: https://bugreports.qt.io/browse/QTBUG-83972, qtwayland cannot auto hide menu
    Connections {
        function onHideMenu() {
            communityContextMenu.close();
        }

        target: MainWindow
    }
    ListView {
        id: communitiesList

        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        model: Communities.filtered()

        ScrollBar.vertical: ScrollBar {
            id: scrollbar

            parent: !collapsed && Settings.scrollbarsInRoomlist ? communitiesList : null
        }
        delegate: ItemDelegate {
            id: communityItem

            property color backgroundColor: palette.window
            property color bubbleBackground: palette.highlight
            property color bubbleText: palette.highlightedText
            property color importantText: palette.text
            required property var model
            property color unimportantText: palette.buttonText

            ToolTip.delay: Nheko.tooltipDelay
            ToolTip.text: model.tooltip
            ToolTip.visible: hovered && collapsed
            height: avatarSize + 2 * Nheko.paddingMedium
            state: "normal"
            width: ListView.view.width - ((scrollbar.interactive && scrollbar.visible && scrollbar.parent) ? scrollbar.width : 0)

            background: Rectangle {
                color: communityItem.backgroundColor
            }
            states: [
                State {
                    name: "highlight"
                    when: (communityItem.hovered || model.hidden) && !(Communities.currentTagId === model.id)

                    PropertyChanges {
                        backgroundColor: palette.dark
                        bubbleBackground: palette.highlight
                        bubbleText: palette.highlightedText
                        importantText: palette.brightText
                        target: communityItem
                        unimportantText: palette.brightText
                    }
                },
                State {
                    name: "selected"
                    when: Communities.currentTagId == model.id

                    PropertyChanges {
                        backgroundColor: palette.highlight
                        bubbleBackground: palette.highlightedText
                        bubbleText: palette.highlight
                        importantText: palette.highlightedText
                        target: communityItem
                        unimportantText: palette.highlightedText
                    }
                }
            ]

            onClicked: Communities.setCurrentTagId(model.id)
            onPressAndHold: communityContextMenu.show(model.id, model.hidden, model.muted)

            Item {
                anchors.fill: parent

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                    gesturePolicy: TapHandler.ReleaseWithinBounds

                    onSingleTapped: communityContextMenu.show(model.id, model.hidden, model.muted)
                }
            }
            RowLayout {
                id: r

                anchors.fill: parent
                anchors.leftMargin: Nheko.paddingMedium + (communitySidebar.collapsed ? 0 : (fontMetrics.lineSpacing * model.depth))
                anchors.margins: Nheko.paddingMedium
                spacing: Nheko.paddingMedium

                ImageButton {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredHeight: fontMetrics.lineSpacing
                    Layout.preferredWidth: fontMetrics.lineSpacing
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: model.collapsed ? qsTr("Expand") : qsTr("Collapse")
                    ToolTip.visible: hovered
                    height: fontMetrics.lineSpacing
                    hoverEnabled: true
                    image: model.collapsed ? ":/icons/icons/ui/collapsed.svg" : ":/icons/icons/ui/expanded.svg"
                    visible: !communitySidebar.collapsed && model.collapsible
                    width: fontMetrics.lineSpacing

                    onClicked: model.collapsed = !model.collapsed
                }
                Item {
                    Layout.preferredWidth: fontMetrics.lineSpacing
                    visible: !communitySidebar.collapsed && !model.collapsible && Communities.containsSubspaces
                }
                Avatar {
                    id: avatar

                    Layout.alignment: Qt.AlignVCenter
                    color: communityItem.backgroundColor
                    displayName: model.displayName
                    enabled: false
                    height: avatarSize
                    roomid: model.id
                    textColor: model.avatarUrl.startsWith(":/") ? communityItem.unimportantText : communityItem.importantText
                    url: {
                        if (model.avatarUrl.startsWith("mxc://"))
                            return model.avatarUrl.replace("mxc://", "image://MxcImage/");
                        else if (model.avatarUrl.length > 0)
                            return model.avatarUrl;
                        else
                            return "";
                    }
                    width: avatarSize

                    NotificationBubble {
                        anchors.bottom: avatar.bottom
                        anchors.margins: -Nheko.paddingSmall
                        anchors.right: avatar.right
                        bubbleBackgroundColor: communityItem.bubbleBackground
                        bubbleTextColor: communityItem.bubbleText
                        font.pixelSize: fontMetrics.font.pixelSize * 0.6
                        hasLoudNotification: model.hasLoudNotification
                        mayBeVisible: communitySidebar.collapsed && !model.muted && Settings.spaceNotifications
                        notificationCount: model.unreadMessages
                    }
                }
                ElidedLabel {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                    color: communityItem.importantText
                    elideWidth: width
                    fullText: model.displayName
                    textFormat: Text.PlainText
                    visible: !communitySidebar.collapsed
                }
                Item {
                    Layout.fillWidth: true
                }
                NotificationBubble {
                    Layout.alignment: Qt.AlignRight
                    Layout.leftMargin: Nheko.paddingSmall
                    bubbleBackgroundColor: communityItem.bubbleBackground
                    bubbleTextColor: communityItem.bubbleText
                    hasLoudNotification: model.hasLoudNotification
                    mayBeVisible: !communitySidebar.collapsed && !model.muted && Settings.spaceNotifications
                    notificationCount: model.unreadMessages
                }
            }
        }

        Platform.Menu {
            id: communityContextMenu

            property bool hidden
            property bool muted
            property string tagId

            function show(id_, hidden_, muted_) {
                tagId = id_;
                hidden = hidden_;
                muted = muted_;
                open();
            }

            Platform.MenuItem {
                checkable: true
                checked: communityContextMenu.muted
                text: qsTr("Do not show notification counts for this community or tag.")

                onTriggered: Communities.toggleTagMute(communityContextMenu.tagId)
            }
            Platform.MenuItem {
                checkable: true
                checked: communityContextMenu.hidden
                text: qsTr("Hide rooms with this tag or from this community by default.")

                onTriggered: Communities.toggleTagId(communityContextMenu.tagId)
            }
        }
    }
}
