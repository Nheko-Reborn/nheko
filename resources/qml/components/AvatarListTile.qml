// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick
import QtQuick.Layouts
import im.nheko

Rectangle {
    id: tile

    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 2.3)
    required property string avatarUrl
    property color background: palette.window
    property color bubbleBackground: palette.highlight
    property color bubbleText: palette.highlightedText
    property bool crop: true
    property color importantText: palette.text
    required property int index
    property alias roomid: avatar.roomid
    required property int selectedIndex
    required property string subtitle
    required property string title
    property color unimportantText: palette.buttonText
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
                tile {
                    background: palette.dark
                    bubbleBackground: palette.highlight
                    bubbleText: palette.highlightedText
                    importantText: palette.brightText
                    unimportantText: palette.brightText
                }
            }
        },
        State {
            name: "selected"
            when: index == selectedIndex

            PropertyChanges {
                tile {
                    background: palette.highlight
                    bubbleBackground: palette.highlightedText
                    bubbleText: palette.highlight
                    importantText: palette.highlightedText
                    unimportantText: palette.highlightedText
                }
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
            implicitHeight: avatarSize
            implicitWidth: avatarSize
            url: tile.avatarUrl.replace("mxc://", "image://MxcImage/")
        }
        ColumnLayout {
            id: textContent

            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            Layout.minimumWidth: 100
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
