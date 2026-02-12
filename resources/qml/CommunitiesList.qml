// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./components"
import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

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
        boundsBehavior: Flickable.StopAtBounds

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

            topInset: 0
            bottomInset: 0
            leftInset: 0
            rightInset: 0

            background: Rectangle {
                color: communityItem.backgroundColor
            }
            states: [
                State {
                    name: "highlight"
                    when: (communityItem.hovered || model.hidden) && !(Communities.currentTagId === model.id)

                    PropertyChanges {
                        communityItem {
                            backgroundColor: palette.dark
                            bubbleBackground: palette.highlight
                            bubbleText: palette.highlightedText
                            importantText: palette.brightText
                            unimportantText: palette.brightText
                        }
                    }
                },
                State {
                    name: "selected"
                    when: Communities.currentTagId == model.id

                    PropertyChanges {
                        communityItem {
                            backgroundColor: palette.highlight
                            bubbleBackground: palette.highlightedText
                            bubbleText: palette.highlight
                            importantText: palette.highlightedText
                            unimportantText: palette.highlightedText
                        }
                    }
                }
            ]

            onClicked: Communities.setCurrentTagId(model.id)
            onPressAndHold: communityContextMenu.show(communityItem, model.id, model.hidden, model.muted)

            Item {
                anchors.fill: parent

                TapHandler {
                    id: rth
                    acceptedButtons: Qt.RightButton
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                    gesturePolicy: TapHandler.ReleaseWithinBounds

                    onSingleTapped: communityContextMenu.show(rth.parent, model.id, model.hidden, model.muted)
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
                    hoverEnabled: true
                    image: model.collapsed ? ":/icons/icons/ui/collapsed.svg" : ":/icons/icons/ui/expanded.svg"
                    visible: !communitySidebar.collapsed && model.collapsible

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
                    Layout.preferredHeight: avatarSize
                    roomid: model.id
                    textColor: model.avatarUrl?.startsWith(":/") == true ? communityItem.unimportantText : communityItem.importantText
                    url: {
                        if (model.avatarUrl?.startsWith("mxc://") == true)
                            return model.avatarUrl.replace("mxc://", "image://MxcImage/");
                        else if ((model.avatarUrl?.length ?? 0) > 0)
                            return model.avatarUrl;
                        else
                            return "";
                    }
                    Layout.preferredWidth: avatarSize

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

        Menu {
            id: communityContextMenu

            property bool hidden
            property bool muted
            property string tagId

            function show(parent, id_, hidden_, muted_) {
                tagId = id_;
                hidden = hidden_;
                muted = muted_;
                popup(parent);
            }

            Component.onCompleted: {
                if (communityContextMenu.popupType != undefined) {
                    communityContextMenu.popupType = 2; // Popup.Native with fallback on older Qt (<6.8.0)
                }
            }


            MenuItem {
                checkable: true
                checked: communityContextMenu.muted
                text: qsTr("Do not show notification counts for this community or tag.")

                onTriggered: Communities.toggleTagMute(communityContextMenu.tagId)
            }
            MenuItem {
                checkable: true
                checked: communityContextMenu.hidden
                text: qsTr("Hide rooms with this tag or from this community by default.")

                onTriggered: Communities.toggleTagId(communityContextMenu.tagId)
            }
        }
    }
}
