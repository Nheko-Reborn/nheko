// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.6
import QtQuick.Controls 2.2
import im.nheko

// This class is for showing Reactions in the timeline row, not for
// adding new reactions via the emoji picker
Flow {
    id: reactionFlow

    property string eventId

    // highlight colors for selfReactedEvent background
    property real highlightHue: timelineRoot.palette.highlight.hslHue
    property real highlightLight: timelineRoot.palette.highlight.hslLightness
    property real highlightSat: timelineRoot.palette.highlight.hslSaturation
    property alias reactions: repeater.model

    spacing: 4

    Repeater {
        id: repeater
        delegate: AbstractButton {
            id: reaction
            ToolTip.delay: Nheko.tooltipDelay
            ToolTip.visible: hovered
            hoverEnabled: true
            implicitHeight: contentItem.childrenRect.height
            implicitWidth: contentItem.childrenRect.width + contentItem.leftPadding * 2

            background: Rectangle {
                anchors.centerIn: parent
                border.color: (reaction.hovered || modelData.selfReactedEvent !== '') ? timelineRoot.palette.highlight : timelineRoot.palette.text
                border.width: 1
                color: modelData.selfReactedEvent !== '' ? Qt.hsla(highlightHue, highlightSat, highlightLight, 0.2) : timelineRoot.palette.window
                implicitHeight: reaction.implicitHeight
                implicitWidth: reaction.implicitWidth
                radius: reaction.height / 2
            }
            contentItem: Row {
                anchors.centerIn: parent
                leftPadding: reactionText.implicitHeight / 2
                rightPadding: reactionText.implicitHeight / 2
                spacing: reactionText.implicitHeight / 4

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
                    color: reaction.hovered ? timelineRoot.palette.highlight : timelineRoot.palette.text
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
                }
                Rectangle {
                    id: divider
                    color: (reaction.hovered || modelData.selfReactedEvent !== '') ? timelineRoot.palette.highlight : timelineRoot.palette.text
                    height: Math.floor(reactionCounter.implicitHeight * 1.4)
                    width: 1
                }
                Text {
                    id: reactionCounter
                    anchors.verticalCenter: divider.verticalCenter
                    color: reaction.hovered ? timelineRoot.palette.highlight : timelineRoot.palette.text
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
