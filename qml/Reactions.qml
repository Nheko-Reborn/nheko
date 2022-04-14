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

    // highlight colors for selfReactedEvent background
    property real highlightHue: timelineRoot.palette.highlight.hslHue
    property real highlightSat: timelineRoot.palette.highlight.hslSaturation
    property real highlightLight: timelineRoot.palette.highlight.hslLightness
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
                    color: reaction.hovered ? timelineRoot.palette.highlight : timelineRoot.palette.text
                    maximumLineCount: 1
                }

                Rectangle {
                    id: divider

                    height: Math.floor(reactionCounter.implicitHeight * 1.4)
                    width: 1
                    color: (reaction.hovered || modelData.selfReactedEvent !== '') ? timelineRoot.palette.highlight : timelineRoot.palette.text
                }

                Text {
                    id: reactionCounter

                    anchors.verticalCenter: divider.verticalCenter
                    text: modelData.count
                    font: reaction.font
                    color: reaction.hovered ? timelineRoot.palette.highlight : timelineRoot.palette.text
                }

            }

            background: Rectangle {
                anchors.centerIn: parent
                implicitWidth: reaction.implicitWidth
                implicitHeight: reaction.implicitHeight
                border.color: (reaction.hovered || modelData.selfReactedEvent !== '') ? timelineRoot.palette.highlight : timelineRoot.palette.text
                color: modelData.selfReactedEvent !== '' ? Qt.hsla(highlightHue, highlightSat, highlightLight, 0.2) : timelineRoot.palette.window
                border.width: 1
                radius: reaction.height / 2
            }

        }

    }

}
