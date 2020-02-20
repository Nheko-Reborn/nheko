import QtQuick 2.6
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

import im.nheko 1.0

import "./delegates"

RowLayout {
	property var view: chat

	anchors.leftMargin: avatarSize + 4
	anchors.left: parent.left
	anchors.right: parent.right

	Column {
		Layout.fillWidth: true
		Layout.alignment: Qt.AlignTop
		spacing: 4

		// fancy reply, if this is a reply
		Reply {
			visible: model.replyTo
			modelData: chat.model.getDump(model.replyTo)
			userColor: timelineManager.userColor(modelData.userId, colors.window)
		}

		// actual message content
		MessageDelegate {
			id: contentItem

			width: parent.width

			modelData: model
		}
	}

	StatusIndicator {
		state: model.state
		Layout.alignment: Qt.AlignRight | Qt.AlignTop
		Layout.preferredHeight: 16
		width: 16
	}

	EncryptionIndicator {
		visible: model.isEncrypted
		Layout.alignment: Qt.AlignRight | Qt.AlignTop
		Layout.preferredHeight: 16
		width: 16
	}

	ImageButton {
		Layout.alignment: Qt.AlignRight | Qt.AlignTop
		Layout.preferredHeight: 16
		width: 16
		id: replyButton
		hoverEnabled: true


		image: ":/icons/icons/ui/mail-reply.png"

		ToolTip.visible: hovered
		ToolTip.text: qsTr("Reply")

		onClicked: view.model.replyAction(model.id)
	}
	ImageButton {
		Layout.alignment: Qt.AlignRight | Qt.AlignTop
		Layout.preferredHeight: 16
		width: 16
		id: optionsButton
		hoverEnabled: true

		image: ":/icons/icons/ui/vertical-ellipsis.png"

		ToolTip.visible: hovered
		ToolTip.text: qsTr("Options")

		onClicked: messageContextMenu.show(model.id, model.type, optionsButton)

	}

	Text {
		Layout.alignment: Qt.AlignRight | Qt.AlignTop
		text: model.timestamp.toLocaleTimeString("HH:mm")
		color: inactiveColors.text

		MouseArea{
			id: ma
			anchors.fill: parent
			hoverEnabled: true
		}

		ToolTip.visible: ma.containsMouse
		ToolTip.text: Qt.formatDateTime(model.timestamp, Qt.DefaultLocaleLongDate)
	}
}
