import QtQuick 2.6
import QtQuick.Controls 2.2

Flow {
	anchors.left: parent.left
	anchors.right: parent.right
	spacing: 4
	Repeater {
		model: ListModel {
			id: nameModel
			ListElement { key: "😊"; count: 5; reactedBySelf: true; users: "Nico, RedSky, AAA, BBB, CCC" }
			ListElement { key: "🤠"; count: 6; reactedBySelf: false; users: "Nico, AAA, BBB, CCC" }
			ListElement { key: "💘"; count: 1; reactedBySelf: true; users: "Nico" }
			ListElement { key: "🙈"; count: 7; reactedBySelf: false; users: "Nico, RedSky, AAA, BBB, CCC, DDD" }
			ListElement { key: "👻"; count: 6; reactedBySelf: false; users: "Nico, RedSky, BBB, CCC" }
		}
		Button {
			id: reaction
			//border.width: 1
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
					font: reaction.font
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
					text: model.count
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
				border.color: (reaction.hovered || model.reactedBySelf )? colors.highlight : colors.buttonText
				color: colors.dark
				border.width: 1
				radius: reaction.height / 2.0
			}
		}
	}
}

