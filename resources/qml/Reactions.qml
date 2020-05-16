import QtQuick 2.6
import QtQuick.Controls 2.2

Flow {
	anchors.left: parent.left
	anchors.right: parent.right
	spacing: 4

	property alias reactions: repeater.model

	Repeater {
		id: repeater

		AbstractButton {
			id: reaction
			hoverEnabled: true
			implicitWidth: contentItem.childrenRect.width + contentItem.leftPadding*2
			implicitHeight: contentItem.childrenRect.height

			ToolTip.visible: hovered
			ToolTip.text: model.users


			contentItem: Row {
				anchors.centerIn: parent
				spacing: reactionText.implicitHeight/4
				leftPadding: reactionText.implicitHeight / 2
				rightPadding: reactionText.implicitHeight / 2

				TextMetrics {
					id: textMetrics
					font.family: settings.emoji_font_family
					elide: Text.ElideRight
					elideWidth: 150
					text: model.key
				}

				Text {
					anchors.baseline: reactionCounter.baseline
					id: reactionText
					text: textMetrics.elidedText + (textMetrics.elidedText == model.key ? "" : "…")
					font.family: settings.emoji_font_family
					color: reaction.hovered ? colors.highlight : colors.text
					maximumLineCount: 1
				}

				Rectangle {
					id: divider
					height: reactionCounter.implicitHeight * 1.4
					width: 1
					color: reaction.hovered ? colors.highlight : colors.text
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
				border.color: (reaction.hovered || model.selfReacted )? colors.highlight : colors.text
				color: colors.base
				border.width: 1
				radius: reaction.height / 2.0
			}
		}
	}
}

