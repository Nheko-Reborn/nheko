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
    property alias reactions: repeater.model
    // Matches the message bubble it's attached to, so the pill reads as part of the same
    // message instead of a separate floating chip. Falls back to the window color for the
    // default (non-bubble) style, which has no bubble to match.
    property color bubbleColor: palette.window

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
                color: reaction.hovered ? palette.highlight : (modelData.selfReactedEvent !== '' ? gentleHighlight : reactionFlow.bubbleColor)
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
                    font.family: Settings.emojiFont != "" ? Settings.emojiFont : undefined
                    font.pointSize: Settings.fontSize * 1.3
                    text: modelData.displayKey
                }
                Text {
                    id: reactionText

                    anchors.verticalCenter: parent.verticalCenter
                    color: (reaction.hovered || modelData.selfReactedEvent !== '') ? palette.highlightedText : palette.text
                    font.family: Settings.emojiFont != "" ? Settings.emojiFont : undefined
                    font.pointSize: Settings.fontSize * 1.3
                    textFormat: TextEdit.RichText
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
                    anchors.verticalCenter: parent.verticalCenter
                    fillMode: Image.PreserveAspectFit
                    height: textMetrics.height
                    mipmap: true
                    source: modelData.key.startsWith("mxc://") ? (modelData.key.replace("mxc://", "image://MxcImage/") + "?scale") : ""
                    visible: modelData.key.startsWith("mxc://")
                    width: textMetrics.height
                }
            }

            // Discreet stand-in for the old count: a small dot instead of a number, so a
            // reaction from several people still reads differently from a single one without
            // a prominent digit competing with the emoji. The exact count is still available
            // via the tooltip on hover.
            Rectangle {
                visible: modelData.count > 1
                width: 6
                height: 6
                radius: 3
                color: (reaction.hovered || modelData.selfReactedEvent !== '') ? palette.highlightedText : palette.highlight
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.bottomMargin: 2
                anchors.rightMargin: 2
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
