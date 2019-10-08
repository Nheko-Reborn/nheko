import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0
import QtQuick.Window 2.2

import com.github.nheko 1.0

import ".."

RowLayout {
	property var view: undefined
	default property alias data: contentItem.data

	height: kid.height // TODO: fix this, we shouldn't need to give the child of contentItem this id!
	anchors.leftMargin: avatarSize + 4
	anchors.left: parent.left
	anchors.right: parent.right
	anchors.rightMargin: scrollbar.width

	function isFullyVisible() {
		return (y - view.contentY - 1) + height < view.height
	}
	function getIndex() {
		return index;
	}

	Item {
		id: contentItem
		Layout.fillWidth: true
		Layout.alignment: Qt.AlignTop
	}

	StatusIndicator {
		state: model.state
		Layout.alignment: Qt.AlignRight | Qt.AlignTop
		Layout.preferredHeight: 16
	}

	EncryptionIndicator {
		visible: model.isEncrypted
		Layout.alignment: Qt.AlignRight | Qt.AlignTop
		Layout.preferredHeight: 16
	}

	Button {
		Layout.alignment: Qt.AlignRight | Qt.AlignTop
		id: replyButton
		flat: true
		Layout.preferredHeight: 16
		ToolTip.visible: hovered
		ToolTip.text: qsTr("Reply")

		// disable background, because we don't want a border on hover
		background: Item {
		}

		Image {
			id: replyButtonImg
			// Workaround, can't get icon.source working for now...
			anchors.fill: parent
			source: "qrc:/icons/icons/ui/mail-reply.png"
		}
		ColorOverlay {
			anchors.fill: replyButtonImg
			source: replyButtonImg
			color: replyButton.hovered ? colors.highlight : colors.buttonText
		}

		onClicked: view.model.replyAction(model.id)
	}
	Button {
		Layout.alignment: Qt.AlignRight | Qt.AlignTop
		id: optionsButton
		flat: true
		Layout.preferredHeight: 16
		ToolTip.visible: hovered
		ToolTip.text: qsTr("Options")

		// disable background, because we don't want a border on hover
		background: Item {
		}

		Image {
			id: optionsButtonImg
			// Workaround, can't get icon.source working for now...
			anchors.fill: parent
			source: "qrc:/icons/icons/ui/vertical-ellipsis.png"
		}
		ColorOverlay {
			anchors.fill: optionsButtonImg
			source: optionsButtonImg
			color: optionsButton.hovered ? colors.highlight : colors.buttonText
		}

		onClicked: contextMenu.open()

		Menu {
			y: optionsButton.height
			id: contextMenu

			MenuItem {
				text: qsTr("Read receipts")
				onTriggered: view.model.readReceiptsAction(model.id)
			}
			MenuItem {
				text: qsTr("Mark as read")
			}
			MenuItem {
				text: qsTr("View raw message")
				onTriggered: view.model.viewRawMessage(model.id)
			}
			MenuItem {
				text: qsTr("Redact message")
				onTriggered: view.model.redactEvent(model.id)
			}
			MenuItem {
				visible: model.type == MtxEvent.ImageMessage || model.type == MtxEvent.VideoMessage || model.type == MtxEvent.AudioMessage || model.type == MtxEvent.FileMessage || model.type == MtxEvent.Sticker
				text: qsTr("Save as")
				onTriggered: timelineManager.saveMedia(model.url, model.filename, model.mimetype, model.type)
			}
		}
	}

	Text {
		Layout.alignment: Qt.AlignRight | Qt.AlignTop
		text: model.timestamp.toLocaleTimeString("HH:mm")
		color: inactiveColors.text

		ToolTip.visible: ma.containsMouse
		ToolTip.text: Qt.formatDateTime(model.timestamp, Qt.DefaultLocaleLongDate)

		MouseArea{
			id: ma
			anchors.fill: parent
			hoverEnabled: true
		}
	}
}
