import QtQuick 2.6
import QtQuick.Controls 2.2

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
	property string roomId

	anchors.left: parent.left
	anchors.right: parent.right
	spacing: 4

	property alias reactions: repeater.model

	Repeater {
		id: repeater

		delegate: AbstractButton {
			id: reaction
			hoverEnabled: true
			implicitWidth: contentItem.childrenRect.width + contentItem.leftPadding*2
			implicitHeight: contentItem.childrenRect.height

			ToolTip.visible: hovered
			ToolTip.text: model.users

			onClicked: {
				console.debug("Picked " + model.key + "in response to " + reactionFlow.eventId + " in room " + reactionFlow.roomId + ". selfReactedEvent: " + model.selfReactedEvent)
				TimelineManager.reactToMessage(reactionFlow.roomId, reactionFlow.eventId, model.key, model.selfReactedEvent)
			}


			contentItem: Row {
				anchors.centerIn: parent
				spacing: reactionText.implicitHeight/4
				leftPadding: reactionText.implicitHeight / 2
				rightPadding: reactionText.implicitHeight / 2

				TextMetrics {
					id: textMetrics
					font.family: Settings.emojiFont
					elide: Text.ElideRight
					elideWidth: 150
					text: model.key
				}

				Text {
					anchors.baseline: reactionCounter.baseline
					id: reactionText
					text: textMetrics.elidedText + (textMetrics.elidedText == model.key ? "" : "…")
					font.family: Settings.emojiFont
					color: reaction.hovered ? colors.highlight : colors.text
					maximumLineCount: 1
				}

				Rectangle {
					id: divider
					height: Math.floor(reactionCounter.implicitHeight * 1.4)
					width: 1
					color: (reaction.hovered || model.selfReactedEvent !== '') ? colors.highlight : colors.text
				}

				Text {
					anchors.verticalCenter: divider.verticalCenter
					id: reactionCounter
					text: model.counter
					font: reaction.font
					color: reaction.hovered ? colors.highlight : colors.text
				}
			}

			background: Rectangle {
				anchors.centerIn: parent

				implicitWidth: reaction.implicitWidth
				implicitHeight: reaction.implicitHeight
				border.color: (reaction.hovered  || model.selfReactedEvent !== '') ? colors.highlight : colors.text
				color: model.selfReactedEvent !== '' ? Qt.hsla(highlightHue, highlightSat, highlightLight, 0.20) : colors.base
				border.width: 1
				radius: reaction.height / 2.0
			}
		}
	}
}

