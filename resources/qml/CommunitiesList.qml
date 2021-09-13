// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./dialogs"
import Qt.labs.platform 1.1 as Platform
import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko 1.0

Page {
    //leftPadding: Nheko.paddingSmall
    //rightPadding: Nheko.paddingSmall
    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 1.6)
    property bool collapsed: false

    ListView {
        id: communitiesList

        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        model: Communities

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

        delegate: Rectangle {
            id: communityItem

            property color background: Nheko.colors.window
            property color importantText: Nheko.colors.text
            property color unimportantText: Nheko.colors.buttonText
            property color bubbleBackground: Nheko.colors.highlight
            property color bubbleText: Nheko.colors.highlightedText

            color: background
            height: avatarSize + 2 * Nheko.paddingMedium
            width: ListView.view.width
            state: "normal"
            ToolTip.visible: hovered.hovered && collapsed
            ToolTip.text: model.tooltip
            states: [
                State {
                    name: "highlight"
                    when: (hovered.hovered || model.hidden) && !(Communities.currentTagId == model.id)

                    PropertyChanges {
                        target: communityItem
                        background: Nheko.colors.dark
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
                        background: Nheko.colors.highlight
                        importantText: Nheko.colors.highlightedText
                        unimportantText: Nheko.colors.highlightedText
                        bubbleBackground: Nheko.colors.highlightedText
                        bubbleText: Nheko.colors.highlight
                    }

                }
            ]

            TapHandler {
                margin: -Nheko.paddingSmall
                acceptedButtons: Qt.RightButton
                onSingleTapped: communityContextMenu.show(model.id)
                gesturePolicy: TapHandler.ReleaseWithinBounds
            }

            TapHandler {
                margin: -Nheko.paddingSmall
                onSingleTapped: Communities.setCurrentTagId(model.id)
                onLongPressed: communityContextMenu.show(model.id)
            }

            HoverHandler {
                id: hovered

                margin: -Nheko.paddingSmall
            }

            RowLayout {
                spacing: Nheko.paddingMedium
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium

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
                    color: communityItem.background
                }

                ElidedLabel {
                    visible: !collapsed
                    Layout.alignment: Qt.AlignVCenter
                    color: communityItem.importantText
                    elideWidth: parent.width - avatar.width - Nheko.paddingMedium
                    fullText: model.displayName
                    textFormat: Text.PlainText
                }

                Item {
                    Layout.fillWidth: true
                }

            }

        }

    }

    background: Rectangle {
        color: Nheko.theme.sidebarBackground
    }

}
