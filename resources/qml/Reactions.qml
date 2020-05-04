import QtQuick 2.6
import QtQuick.Controls 2.2

Flow {
	anchors.left: parent.left
	anchors.right: parent.right
	spacing: 4

	property alias reactions: repeater.model

	Repeater {
		id: repeater

		Button {
			id: reaction
			text: model.key
			hoverEnabled: true
			implicitWidth: contentItem.childrenRect.width + contentItem.padding*2
			implicitHeight: contentItem.childrenRect.height + contentItem.padding*2

			ToolTip.visible: hovered
			ToolTip.text: model.users

			contentItem: Row {
				anchors.centerIn: parent
				spacing: 2
				padding: 4

				Text {
					id: reactionText
					text: reaction.text
					font.family: settings.emoji_font_family
					opacity: enabled ? 1.0 : 0.3
					color: reaction.hovered ? colors.highlight : colors.buttonText
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
					elide: Text.ElideRight
				}

				Rectangle {
					height: reactionText.implicitHeight
					width: 1
					color: reaction.hovered ? colors.highlight : colors.buttonText
				}

				Text {
					text: model.counter
					font: reaction.font
					opacity: enabled ? 1.0 : 0.3
					color: reaction.hovered ? colors.highlight : colors.buttonText
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
					elide: Text.ElideRight
				}
			}

			background: Rectangle {
				anchors.centerIn: parent
				implicitWidth: reaction.implicitWidth
				implicitHeight: reaction.implicitHeight
				opacity: enabled ? 1 : 0.3
				border.color: (reaction.hovered || model.selfReacted )? colors.highlight : colors.buttonText
				color: colors.dark
				border.width: 1
				radius: reaction.height / 2.0
			}
		}
	}
}

