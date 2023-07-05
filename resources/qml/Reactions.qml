// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.6
import QtQuick.Controls 2.2
import im.nheko 1.0

// This class is for showing Reactions in the timeline row, not for
// adding new reactions via the emoji picker
Flow {
    id: reactionFlow

    property string eventId

    // lower-contrast colors to avoid distracting from text & to enhance hover effect
    property color gentleHighlight: Qt.hsla(palette.highlight.hslHue, palette.highlight.hslSaturation, palette.highlight.hslLightness, 0.8)
    property color gentleText: Qt.hsla(palette.text.hslHue, palette.text.hslSaturation, palette.text.hslLightness, 0.6)
    property alias reactions: repeater.model

    spacing: 4

    Repeater {
        id: repeater

        delegate: AbstractButton {
            id: reaction

            ToolTip.delay: Nheko.tooltipDelay
            ToolTip.visible: hovered
            hoverEnabled: true
            leftPadding: textMetrics.height / 2
            rightPadding: textMetrics.height / 2

            background: Rectangle {
                anchors.centerIn: parent
                border.color: reaction.hovered ? palette.text : gentleText
                border.width: 1
                color: reaction.hovered ? palette.highlight : (modelData.selfReactedEvent !== '' ? gentleHighlight : palette.window)
                implicitHeight: reaction.implicitHeight
                implicitWidth: reaction.implicitWidth
                radius: reaction.height / 2
            }
            contentItem: Row {
                spacing: textMetrics.height / 4

                TextMetrics {
                    id: textMetrics

                    elide: Text.ElideRight
                    elideWidth: 150
                    font.family: Settings.emojiFont
                    text: modelData.displayKey
                }
                Text {
                    id: reactionText

                    anchors.baseline: reactionCounter.baseline
                    color: (reaction.hovered || modelData.selfReactedEvent !== '') ? palette.highlightedText : palette.text
                    font.family: Settings.emojiFont
                    maximumLineCount: 1
                    text: {
                        // When an emoji font is selected that doesn't have …, it is dropped from elidedText. So we add it back.
                        if (textMetrics.elidedText !== modelData.displayKey) {
                            if (!textMetrics.elidedText.endsWith("…")) {
                                return textMetrics.elidedText + "…";
                            }
                        }
                        return textMetrics.elidedText;
                    }
                    visible: !modelData.key.startsWith("mxc://")
                }
                Image {
                    anchors.verticalCenter: divider.verticalCenter
                    fillMode: Image.PreserveAspectFit
                    height: textMetrics.height
                    source: modelData.key.startsWith("mxc://") ? (modelData.key.replace("mxc://", "image://MxcImage/") + "?scale") : ""
                    visible: modelData.key.startsWith("mxc://")
                    width: textMetrics.height
                }
                Rectangle {
                    id: divider

                    color: reaction.hovered ? palette.text : gentleText
                    height: Math.floor(reactionCounter.implicitHeight * 1.4)
                    width: 1
                }
                Text {
                    id: reactionCounter

                    anchors.verticalCenter: divider.verticalCenter
                    color: (reaction.hovered || modelData.selfReactedEvent !== '') ? palette.highlightedText : palette.windowText
                    font: reaction.font
                    text: modelData.count
                }
            }

            Component.onCompleted: {
                ToolTip.text = Qt.binding(function () {
                        if (textMetrics.elidedText === textMetrics.text) {
                            return modelData.users;
                        }
                        return modelData.displayKey + "\n" + modelData.users;
                    });
            }
            onClicked: {
                console.debug("Picked " + modelData.key + "in response to " + reactionFlow.eventId + ". selfReactedEvent: " + modelData.selfReactedEvent);
                room.input.reaction(reactionFlow.eventId, modelData.key);
            }
        }
    }
}
