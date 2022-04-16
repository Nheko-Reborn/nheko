// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "dialogs"
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

    background: Rectangle {
        color: Nheko.theme.sidebarBackground
    }

    ListView {
        id: communitiesList
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        model: Communities.filtered()

        delegate: ItemDelegate {
            id: communityItem

            property color backgroundColor: timelineRoot.palette.window
            property color bubbleBackground: timelineRoot.palette.highlight
            property color bubbleText: timelineRoot.palette.highlightedText
            property color importantText: timelineRoot.palette.text
            property color unimportantText: timelineRoot.palette.placeholderText

            ToolTip.delay: Nheko.tooltipDelay
            ToolTip.text: model.tooltip
            ToolTip.visible: hovered && collapsed
            height: avatarSize + 2 * Nheko.paddingMedium
            state: "normal"
            width: ListView.view.width

            background: Rectangle {
                color: backgroundColor
            }
            states: [
                State {
                    name: "highlight"
                    when: (communityItem.hovered || model.hidden) && !(Communities.currentTagId == model.id)

                    PropertyChanges {
                        backgroundColor: timelineRoot.palette.dark
                        bubbleBackground: timelineRoot.palette.highlight
                        bubbleText: timelineRoot.palette.highlightedText
                        importantText: timelineRoot.palette.brightText
                        target: communityItem
                        unimportantText: timelineRoot.palette.brightText
                    }
                },
                State {
                    name: "selected"
                    when: Communities.currentTagId == model.id

                    PropertyChanges {
                        backgroundColor: timelineRoot.palette.highlight
                        bubbleBackground: timelineRoot.palette.highlightedText
                        bubbleText: timelineRoot.palette.highlight
                        importantText: timelineRoot.palette.highlightedText
                        target: communityItem
                        unimportantText: timelineRoot.palette.highlightedText
                    }
                }
            ]

            onClicked: Communities.setCurrentTagId(model.id)
            onPressAndHold: communityContextMenu.show(model.id)

            Item {
                anchors.fill: parent

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                    gesturePolicy: TapHandler.ReleaseWithinBounds

                    onSingleTapped: communityContextMenu.show(model.id)
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
                    url: {
                        if (model.avatarUrl.startsWith("mxc://"))
                            return model.avatarUrl.replace("mxc://", "image://MxcImage/");
                        else
                            return "image://colorimage/" + model.avatarUrl + "?" + communityItem.unimportantText;
                    }
                    width: avatarSize
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
            }
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
    }
}
