// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

Rectangle {
    id: tile

    property color background: Nheko.colors.window
    property color importantText: Nheko.colors.text
    property color unimportantText: Nheko.colors.buttonText
    property color bubbleBackground: Nheko.colors.highlight
    property color bubbleText: Nheko.colors.highlightedText
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
                background: Nheko.colors.dark
                importantText: Nheko.colors.brightText
                unimportantText: Nheko.colors.brightText
                bubbleBackground: Nheko.colors.highlight
                bubbleText: Nheko.colors.highlightedText
            }

        },
        State {
            name: "selected"
            when: index == selectedIndex

            PropertyChanges {
                target: tile
                background: Nheko.colors.highlight
                importantText: Nheko.colors.highlightedText
                unimportantText: Nheko.colors.highlightedText
                bubbleBackground: Nheko.colors.highlightedText
                bubbleText: Nheko.colors.highlight
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
