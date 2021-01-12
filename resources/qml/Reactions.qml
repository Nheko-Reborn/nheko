import QtQuick 2.10
import QtQuick.Controls 2.3
import im.nheko 1.0

// This class is for showing Reactions in the timeline row, not for
// adding new reactions via the emoji picker
Flow {
    id: reactionFlow

    // highlight colors for selfReactedEvent background
    property real highlightHue: colors.highlight.hslHue
    property real highlightSat: colors.highlight.hslSaturation
    property real highlightLight: colors.highlight.hslLightness
    property string eventId
    property alias reactions: repeater.model

    anchors.left: parent.left
    anchors.right: parent.right
    spacing: 4

    Repeater {
        id: repeater

        delegate: AbstractButton {
            id: reaction

            hoverEnabled: true
            implicitWidth: contentItem.childrenRect.width + contentItem.leftPadding * 2
            implicitHeight: contentItem.childrenRect.height
            ToolTip.visible: hovered
            ToolTip.text: modelData.users
            onClicked: {
                console.debug("Picked " + modelData.key + "in response to " + reactionFlow.eventId + ". selfReactedEvent: " + modelData.selfReactedEvent);
                TimelineManager.queueReactionMessage(reactionFlow.eventId, modelData.key);
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
                    text: modelData.key
                }

                Text {
                    id: reactionText

                    anchors.baseline: reactionCounter.baseline
                    text: textMetrics.elidedText + (textMetrics.elidedText == modelData.key ? "" : "…")
                    font.family: Settings.emojiFont
                    color: reaction.hovered ? colors.highlight : colors.text
                    maximumLineCount: 1
                }

                Rectangle {
                    id: divider

                    height: Math.floor(reactionCounter.implicitHeight * 1.4)
                    width: 1
                    color: (reaction.hovered || modelData.selfReactedEvent !== '') ? colors.highlight : colors.text
                }

                Text {
                    id: reactionCounter

                    anchors.verticalCenter: divider.verticalCenter
                    text: modelData.count
                    font: reaction.font
                    color: reaction.hovered ? colors.highlight : colors.text
                }

            }

            background: Rectangle {
                anchors.centerIn: parent
                implicitWidth: reaction.implicitWidth
                implicitHeight: reaction.implicitHeight
                border.color: (reaction.hovered || modelData.selfReactedEvent !== '') ? colors.highlight : colors.text
                color: modelData.selfReactedEvent !== '' ? Qt.hsla(highlightHue, highlightSat, highlightLight, 0.2) : colors.window
                border.width: 1
                radius: reaction.height / 2
            }

        }

    }

}
