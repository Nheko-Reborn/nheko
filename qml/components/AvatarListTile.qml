// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko

Rectangle {
    id: tile

    property color background: timelineRoot.palette.window
    property color importantText: timelineRoot.palette.text
    property color unimportantText: timelineRoot.palette.buttonText
    property color bubbleBackground: timelineRoot.palette.highlight
    property color bubbleText: timelineRoot.palette.highlightedText
    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 2.3)
    required property string avatarUrl
    required property string title
    required property string subtitle
    required property int index
    required property int selectedIndex
    property bool crop: true
    property alias roomid: avatar.roomid
    property alias userid: avatar.userid

    color: background
    height: avatarSize + 2 * Nheko.paddingMedium
    width: ListView.view.width
    state: "normal"
    states: [
        State {
            name: "highlight"
            when: hovered.hovered && !(index == selectedIndex)

            PropertyChanges {
                target: tile
                background: timelineRoot.palette.dark
                importantText: timelineRoot.palette.brightText
                unimportantText: timelineRoot.palette.brightText
                bubbleBackground: timelineRoot.palette.highlight
                bubbleText: timelineRoot.palette.highlightedText
            }

        },
        State {
            name: "selected"
            when: index == selectedIndex

            PropertyChanges {
                target: tile
                background: timelineRoot.palette.highlight
                importantText: timelineRoot.palette.highlightedText
                unimportantText: timelineRoot.palette.highlightedText
                bubbleBackground: timelineRoot.palette.highlightedText
                bubbleText: timelineRoot.palette.highlight
            }

        }
    ]

    HoverHandler {
        id: hovered
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
            url: tile.avatarUrl.replace("mxc://", "image://MxcImage/")
            displayName: title
            crop: tile.crop
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
                    color: tile.importantText
                    elideWidth: textContent.width - Nheko.paddingMedium
                    fullText: title
                    textFormat: Text.PlainText
                }

                Item {
                    Layout.fillWidth: true
                }

            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                ElidedLabel {
                    color: tile.unimportantText
                    font.pixelSize: fontMetrics.font.pixelSize * 0.9
                    elideWidth: textContent.width - Nheko.paddingSmall
                    fullText: subtitle
                    textFormat: Text.PlainText
                }

                Item {
                    Layout.fillWidth: true
                }

            }

        }

    }

}
