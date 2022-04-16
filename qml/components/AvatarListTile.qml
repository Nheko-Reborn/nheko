// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko

Rectangle {
    id: tile

    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 2.3)
    required property string avatarUrl
    property color background: timelineRoot.palette.window
    property color bubbleBackground: timelineRoot.palette.highlight
    property color bubbleText: timelineRoot.palette.highlightedText
    property bool crop: true
    property color importantText: timelineRoot.palette.text
    required property int index
    property alias roomid: avatar.roomid
    required property int selectedIndex
    required property string subtitle
    required property string title
    property color unimportantText: timelineRoot.palette.placeholderText
    property alias userid: avatar.userid

    color: background
    height: avatarSize + 2 * Nheko.paddingMedium
    state: "normal"
    width: ListView.view.width

    states: [
        State {
            name: "highlight"
            when: hovered.hovered && !(index == selectedIndex)

            PropertyChanges {
                background: timelineRoot.palette.dark
                bubbleBackground: timelineRoot.palette.highlight
                bubbleText: timelineRoot.palette.highlightedText
                importantText: timelineRoot.palette.brightText
                target: tile
                unimportantText: timelineRoot.palette.brightText
            }
        },
        State {
            name: "selected"
            when: index == selectedIndex

            PropertyChanges {
                background: timelineRoot.palette.highlight
                bubbleBackground: timelineRoot.palette.highlightedText
                bubbleText: timelineRoot.palette.highlight
                importantText: timelineRoot.palette.highlightedText
                target: tile
                unimportantText: timelineRoot.palette.highlightedText
            }
        }
    ]

    HoverHandler {
        id: hovered
    }
    RowLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Avatar {
            id: avatar
            Layout.alignment: Qt.AlignVCenter
            crop: tile.crop
            displayName: title
            enabled: false
            height: avatarSize
            url: tile.avatarUrl.replace("mxc://", "image://MxcImage/")
            width: avatarSize
        }
        ColumnLayout {
            id: textContent
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            Layout.preferredWidth: parent.width - avatar.width
            spacing: Nheko.paddingSmall
            width: parent.width - avatar.width

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
                    elideWidth: textContent.width - Nheko.paddingSmall
                    font.pixelSize: fontMetrics.font.pixelSize * 0.9
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
