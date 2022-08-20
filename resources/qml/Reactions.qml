// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.6
import QtQuick.Controls 2.2
import im.nheko 1.0

// This class is for showing Reactions in the timeline row, not for
// adding new reactions via the emoji picker
Flow {
    id: reactionFlow

    // lower-contrast colors to avoid distracting from text & to enhance hover effect
    property color gentleHighlight: Qt.hsla(Nheko.colors.highlight.hslHue, Nheko.colors.highlight.hslSaturation, Nheko.colors.highlight.hslLightness, 0.8)
    property color gentleText: Qt.hsla(Nheko.colors.text.hslHue, Nheko.colors.text.hslSaturation, Nheko.colors.text.hslLightness, 0.6)
    property string eventId
    property alias reactions: repeater.model

    spacing: 4

    Repeater {
        id: repeater

        delegate: AbstractButton {
            id: reaction

            hoverEnabled: true
            implicitWidth: contentItem.childrenRect.width + contentItem.leftPadding * 2
            implicitHeight: contentItem.childrenRect.height
            ToolTip.visible: hovered
            ToolTip.delay: Nheko.tooltipDelay
            onClicked: {
                console.debug("Picked " + modelData.key + "in response to " + reactionFlow.eventId + ". selfReactedEvent: " + modelData.selfReactedEvent);
                room.input.reaction(reactionFlow.eventId, modelData.key);
            }
            Component.onCompleted: {
                ToolTip.text = Qt.binding(function() {
                    if (textMetrics.elidedText === textMetrics.text) {
                        return modelData.users;
                    }
                    return modelData.displayKey + "\n" + modelData.users;
                })
            }

            contentItem: Row {
                anchors.centerIn: parent
                spacing: reactionText.implicitHeight / 4
                leftPadding: reactionText.implicitHeight / 2
                rightPadding: reactionText.implicitHeight / 2

                TextMetrics {
                    id: textMetrics

                    font.family: Settings.emojiFont
                    elide: Text.ElideRight
                    elideWidth: 150
                    text: modelData.displayKey
                }

                Text {
                    id: reactionText

                    anchors.baseline: reactionCounter.baseline
                    text: {
                        // When an emoji font is selected that doesn't have …, it is dropped from elidedText. So we add it back.
                        if (textMetrics.elidedText !== modelData.displayKey) {
                            if (!textMetrics.elidedText.endsWith("…")) {
                                return textMetrics.elidedText + "…";
                            }
                        }
                        return textMetrics.elidedText;
                    }
                    font.family: Settings.emojiFont
                    color: (reaction.hovered || modelData.selfReactedEvent !== '') ? Nheko.colors.highlightedText: Nheko.colors.text
                    maximumLineCount: 1
                }

                Rectangle {
                    id: divider

                    height: Math.floor(reactionCounter.implicitHeight * 1.4)
                    width: 1
                    color: reaction.hovered ? Nheko.colors.text: gentleText
                }

                Text {
                    id: reactionCounter

                    anchors.verticalCenter: divider.verticalCenter
                    text: modelData.count
                    font: reaction.font
                    color: (reaction.hovered || modelData.selfReactedEvent !== '') ? Nheko.colors.highlightedText: Nheko.colors.windowText
                }

            }

            background: Rectangle {
                anchors.centerIn: parent
                implicitWidth: reaction.implicitWidth
                implicitHeight: reaction.implicitHeight
                border.color: reaction.hovered ? Nheko.colors.text: gentleText
                color: reaction.hovered ? Nheko.colors.highlight : (modelData.selfReactedEvent !== '' ? gentleHighlight : Nheko.colors.window)
                border.width: 1
                radius: reaction.height / 2
            }

        }

    }

}
