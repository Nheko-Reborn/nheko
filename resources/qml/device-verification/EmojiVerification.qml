// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10
import im.nheko 1.0

ColumnLayout {
    property string title: qsTr("Verification Code")

    spacing: 16

    Label {
        Layout.preferredWidth: 400
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        text: qsTr("Please verify the following emoji. You should see the same emoji on both sides. If they differ, please press 'They do not match!' to abort verification!")
        color: Nheko.colors.text
        verticalAlignment: Text.AlignVCenter
    }

    Item { Layout.fillHeight: true; }
    RowLayout {
        id: emojis

        property var mapping: [{
            "number": 0,
            "emoji": "🐶",
            "description": "Dog",
            "unicode": "U+1F436"
        }, {
            "number": 1,
            "emoji": "🐱",
            "description": "Cat",
            "unicode": "U+1F431"
        }, {
            "number": 2,
            "emoji": "🦁",
            "description": "Lion",
            "unicode": "U+1F981"
        }, {
            "number": 3,
            "emoji": "🐎",
            "description": "Horse",
            "unicode": "U+1F40E"
        }, {
            "number": 4,
            "emoji": "🦄",
            "description": "Unicorn",
            "unicode": "U+1F984"
        }, {
            "number": 5,
            "emoji": "🐷",
            "description": "Pig",
            "unicode": "U+1F437"
        }, {
            "number": 6,
            "emoji": "🐘",
            "description": "Elephant",
            "unicode": "U+1F418"
        }, {
            "number": 7,
            "emoji": "🐰",
            "description": "Rabbit",
            "unicode": "U+1F430"
        }, {
            "number": 8,
            "emoji": "🐼",
            "description": "Panda",
            "unicode": "U+1F43C"
        }, {
            "number": 9,
            "emoji": "🐓",
            "description": "Rooster",
            "unicode": "U+1F413"
        }, {
            "number": 10,
            "emoji": "🐧",
            "description": "Penguin",
            "unicode": "U+1F427"
        }, {
            "number": 11,
            "emoji": "🐢",
            "description": "Turtle",
            "unicode": "U+1F422"
        }, {
            "number": 12,
            "emoji": "🐟",
            "description": "Fish",
            "unicode": "U+1F41F"
        }, {
            "number": 13,
            "emoji": "🐙",
            "description": "Octopus",
            "unicode": "U+1F419"
        }, {
            "number": 14,
            "emoji": "🦋",
            "description": "Butterfly",
            "unicode": "U+1F98B"
        }, {
            "number": 15,
            "emoji": "🌷",
            "description": "Flower",
            "unicode": "U+1F337"
        }, {
            "number": 16,
            "emoji": "🌳",
            "description": "Tree",
            "unicode": "U+1F333"
        }, {
            "number": 17,
            "emoji": "🌵",
            "description": "Cactus",
            "unicode": "U+1F335"
        }, {
            "number": 18,
            "emoji": "🍄",
            "description": "Mushroom",
            "unicode": "U+1F344"
        }, {
            "number": 19,
            "emoji": "🌏",
            "description": "Globe",
            "unicode": "U+1F30F"
        }, {
            "number": 20,
            "emoji": "🌙",
            "description": "Moon",
            "unicode": "U+1F319"
        }, {
            "number": 21,
            "emoji": "☁️",
            "description": "Cloud",
            "unicode": "U+2601U+FE0F"
        }, {
            "number": 22,
            "emoji": "🔥",
            "description": "Fire",
            "unicode": "U+1F525"
        }, {
            "number": 23,
            "emoji": "🍌",
            "description": "Banana",
            "unicode": "U+1F34C"
        }, {
            "number": 24,
            "emoji": "🍎",
            "description": "Apple",
            "unicode": "U+1F34E"
        }, {
            "number": 25,
            "emoji": "🍓",
            "description": "Strawberry",
            "unicode": "U+1F353"
        }, {
            "number": 26,
            "emoji": "🌽",
            "description": "Corn",
            "unicode": "U+1F33D"
        }, {
            "number": 27,
            "emoji": "🍕",
            "description": "Pizza",
            "unicode": "U+1F355"
        }, {
            "number": 28,
            "emoji": "🎂",
            "description": "Cake",
            "unicode": "U+1F382"
        }, {
            "number": 29,
            "emoji": "❤️",
            "description": "Heart",
            "unicode": "U+2764U+FE0F"
        }, {
            "number": 30,
            "emoji": "😀",
            "description": "Smiley",
            "unicode": "U+1F600"
        }, {
            "number": 31,
            "emoji": "🤖",
            "description": "Robot",
            "unicode": "U+1F916"
        }, {
            "number": 32,
            "emoji": "🎩",
            "description": "Hat",
            "unicode": "U+1F3A9"
        }, {
            "number": 33,
            "emoji": "👓",
            "description": "Glasses",
            "unicode": "U+1F453"
        }, {
            "number": 34,
            "emoji": "🔧",
            "description": "Spanner",
            "unicode": "U+1F527"
        }, {
            "number": 35,
            "emoji": "🎅",
            "description": "Santa",
            "unicode": "U+1F385"
        }, {
            "number": 36,
            "emoji": "👍",
            "description": "Thumbs Up",
            "unicode": "U+1F44D"
        }, {
            "number": 37,
            "emoji": "☂️",
            "description": "Umbrella",
            "unicode": "U+2602U+FE0F"
        }, {
            "number": 38,
            "emoji": "⌛",
            "description": "Hourglass",
            "unicode": "U+231B"
        }, {
            "number": 39,
            "emoji": "⏰",
            "description": "Clock",
            "unicode": "U+23F0"
        }, {
            "number": 40,
            "emoji": "🎁",
            "description": "Gift",
            "unicode": "U+1F381"
        }, {
            "number": 41,
            "emoji": "💡",
            "description": "Light Bulb",
            "unicode": "U+1F4A1"
        }, {
            "number": 42,
            "emoji": "📕",
            "description": "Book",
            "unicode": "U+1F4D5"
        }, {
            "number": 43,
            "emoji": "✏️",
            "description": "Pencil",
            "unicode": "U+270FU+FE0F"
        }, {
            "number": 44,
            "emoji": "📎",
            "description": "Paperclip",
            "unicode": "U+1F4CE"
        }, {
            "number": 45,
            "emoji": "✂️",
            "description": "Scissors",
            "unicode": "U+2702U+FE0F"
        }, {
            "number": 46,
            "emoji": "🔒",
            "description": "Lock",
            "unicode": "U+1F512"
        }, {
            "number": 47,
            "emoji": "🔑",
            "description": "Key",
            "unicode": "U+1F511"
        }, {
            "number": 48,
            "emoji": "🔨",
            "description": "Hammer",
            "unicode": "U+1F528"
        }, {
            "number": 49,
            "emoji": "☎️",
            "description": "Telephone",
            "unicode": "U+260EU+FE0F"
        }, {
            "number": 50,
            "emoji": "🏁",
            "description": "Flag",
            "unicode": "U+1F3C1"
        }, {
            "number": 51,
            "emoji": "🚂",
            "description": "Train",
            "unicode": "U+1F682"
        }, {
            "number": 52,
            "emoji": "🚲",
            "description": "Bicycle",
            "unicode": "U+1F6B2"
        }, {
            "number": 53,
            "emoji": "✈️",
            "description": "Aeroplane",
            "unicode": "U+2708U+FE0F"
        }, {
            "number": 54,
            "emoji": "🚀",
            "description": "Rocket",
            "unicode": "U+1F680"
        }, {
            "number": 55,
            "emoji": "🏆",
            "description": "Trophy",
            "unicode": "U+1F3C6"
        }, {
            "number": 56,
            "emoji": "⚽",
            "description": "Ball",
            "unicode": "U+26BD"
        }, {
            "number": 57,
            "emoji": "🎸",
            "description": "Guitar",
            "unicode": "U+1F3B8"
        }, {
            "number": 58,
            "emoji": "🎺",
            "description": "Trumpet",
            "unicode": "U+1F3BA"
        }, {
            "number": 59,
            "emoji": "🔔",
            "description": "Bell",
            "unicode": "U+1F514"
        }, {
            "number": 60,
            "emoji": "⚓",
            "description": "Anchor",
            "unicode": "U+2693"
        }, {
            "number": 61,
            "emoji": "🎧",
            "description": "Headphones",
            "unicode": "U+1F3A7"
        }, {
            "number": 62,
            "emoji": "📁",
            "description": "Folder",
            "unicode": "U+1F4C1"
        }, {
            "number": 63,
            "emoji": "📌",
            "description": "Pin",
            "unicode": "U+1F4CC"
        }]

        Layout.alignment: Qt.AlignHCenter

        Repeater {
            id: repeater

            model: 7

            delegate: Rectangle {
                color: "transparent"
                implicitHeight: Qt.application.font.pixelSize * 8
                implicitWidth: col.width

                ColumnLayout {
                    id: col

                    property var emoji: emojis.mapping[flow.sasList[index]]

                    Layout.fillWidth: true
                    anchors.bottom: parent.bottom

                    Label {
                        //height: font.pixelSize * 2
                        Layout.alignment: Qt.AlignHCenter
                        text: col.emoji.emoji
                        font.pixelSize: Qt.application.font.pixelSize * 2
                        font.family: Settings.emojiFont
                        color: Nheko.colors.text
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                        text: col.emoji.description
                        color: Nheko.colors.text
                    }

                }

            }

        }

    }
    Item { Layout.fillHeight: true; }

    Label {
        Layout.preferredWidth: 400
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        text: qsTr("The displayed emoji might look different in different clients if a different font is used. Similarly they might be translated into different languages. Nonetheless they should depict one of 64 different objects or animals. For example a lion and a cat are different, but a cat is the same even if one client just shows a cat face, while another client shows a full cat body.")
        color: Nheko.colors.text
        verticalAlignment: Text.AlignVCenter
    }

    Item { Layout.fillHeight: true; }

    RowLayout {
        Button {
            Layout.alignment: Qt.AlignLeft
            text: qsTr("They do not match!")
            onClicked: {
                flow.cancel();
                dialog.close();
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("They match!")
            onClicked: flow.next()
        }

    }

}
