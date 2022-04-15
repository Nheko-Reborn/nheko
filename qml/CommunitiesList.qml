// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./dialogs"
import Qt.labs.platform 1.1 as Platform
import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko

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

            property color backgroundColor: timelineRoot.palette.window
            property color importantText: timelineRoot.palette.text
            property color unimportantText: timelineRoot.palette.buttonText
            property color bubbleBackground: timelineRoot.palette.highlight
            property color bubbleText: timelineRoot.palette.highlightedText

            height: avatarSize + 2 * Nheko.paddingMedium
            width: ListView.view.width
            state: "normal"
            ToolTip.visible: hovered && collapsed
            ToolTip.text: model.tooltip
            ToolTip.delay: Nheko.tooltipDelay
            onClicked: Communities.setCurrentTagId(model.id)
            onPressAndHold: communityContextMenu.show(model.id)
            states: [
                State {
                    name: "highlight"
                    when: (communityItem.hovered || model.hidden) && !(Communities.currentTagId == model.id)

                    PropertyChanges {
                        target: communityItem
                        backgroundColor: timelineRoot.palette.dark
                        importantText: timelineRoot.palette.brightText
                        unimportantText: timelineRoot.palette.brightText
                        bubbleBackground: timelineRoot.palette.highlight
                        bubbleText: timelineRoot.palette.highlightedText
                    }

                },
                State {
                    name: "selected"
                    when: Communities.currentTagId == model.id

                    PropertyChanges {
                        target: communityItem
                        backgroundColor: timelineRoot.palette.highlight
                        importantText: timelineRoot.palette.highlightedText
                        unimportantText: timelineRoot.palette.highlightedText
                        bubbleBackground: timelineRoot.palette.highlightedText
                        bubbleText: timelineRoot.palette.highlight
                    }

                }
            ]

            Item {
                anchors.fill: parent

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onSingleTapped: communityContextMenu.show(model.id)
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

            }

            background: Rectangle {
                color: backgroundColor
            }

        }

    }

    background: Rectangle {
        color: Nheko.theme.sidebarBackground
    }

}
