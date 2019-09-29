import QtQuick 2.6
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.5
import QtGraphicalEffects 1.0
import QtQuick.Window 2.2

import com.github.nheko 1.0

Rectangle {
	anchors.fill: parent

	SystemPalette { id: colors; colorGroup: SystemPalette.Active }
	SystemPalette { id: inactiveColors; colorGroup: SystemPalette.Disabled }
	property int avatarSize: 32

	color: colors.window

	Text {
		visible: !timelineManager.timeline
		anchors.centerIn: parent
		text: qsTr("No room open")
		font.pointSize: 24
		color: colors.windowText
	}

	ListView {
		id: chat

		cacheBuffer: parent.height

		visible: timelineManager.timeline != null
		anchors.fill: parent

		model: timelineManager.timeline

		onModelChanged: {
			if (model) {
				currentIndex = model.currentIndex
				if (model.currentIndex == count - 1) {
					positionViewAtEnd()
				} else {
					positionViewAtIndex(model.currentIndex, ListView.End)
				}
			}
		}

		ScrollBar.vertical: ScrollBar {
			id: scrollbar
			anchors.top: parent.top
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			onPressedChanged: if (!pressed) chat.updatePosition()
		}

		property bool atBottom: false
		onCountChanged: {
			if (atBottom && Window.active) {
				var newIndex = count - 1 // last index
				positionViewAtEnd()
				currentIndex = newIndex
				model.currentIndex = newIndex
			}
		}

		function updatePosition() {
			for (var y = chat.contentY + chat.height; y > chat.height; y -= 5) {
				var i = chat.itemAt(100, y);
				if (!i) continue;
				if (!i.isFullyVisible()) continue;
				chat.model.currentIndex = i.getIndex();
				chat.currentIndex = i.getIndex()
				atBottom = i.getIndex() == count - 1;
				console.log("bottom:" + atBottom)
				break;
			}
		}
		onMovementEnded: updatePosition()

		spacing: 4
		delegate: RowLayout {
			anchors.leftMargin: avatarSize + 4
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.rightMargin: scrollbar.width

			function isFullyVisible() {
				return (y - chat.contentY - 1) + height < chat.height
			}
			function getIndex() {
				return index;
			}

			Loader {
				id: loader
				Layout.fillWidth: true
				Layout.alignment: Qt.AlignTop
				height: item.height

				source: switch(model.type) {
					//case MtxEvent.Aliases: return "delegates/Aliases.qml"
					//case MtxEvent.Avatar: return "delegates/Avatar.qml"
					//case MtxEvent.CanonicalAlias: return "delegates/CanonicalAlias.qml"
					//case MtxEvent.Create: return "delegates/Create.qml"
					//case MtxEvent.GuestAccess: return "delegates/GuestAccess.qml"
					//case MtxEvent.HistoryVisibility: return "delegates/HistoryVisibility.qml"
					//case MtxEvent.JoinRules: return "delegates/JoinRules.qml"
					//case MtxEvent.Member: return "delegates/Member.qml"
					//case MtxEvent.Name: return "delegates/Name.qml"
					//case MtxEvent.PowerLevels: return "delegates/PowerLevels.qml"
					//case MtxEvent.Topic: return "delegates/Topic.qml"
					case MtxEvent.NoticeMessage: return "delegates/NoticeMessage.qml"
					case MtxEvent.TextMessage: return "delegates/TextMessage.qml"
					case MtxEvent.ImageMessage: return "delegates/ImageMessage.qml"
					//case MtxEvent.VideoMessage: return "delegates/VideoMessage.qml"
					case MtxEvent.Redacted: return "delegates/Redacted.qml"
					default: return "delegates/placeholder.qml"
				}
				property variant eventData: model
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

				onClicked: chat.model.replyAction(model.id)
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
						onTriggered: chat.model.readReceiptsAction(model.id)
					}
					MenuItem {
						text: qsTr("Mark as read")
					}
					MenuItem {
						text: qsTr("View raw message")
						onTriggered: chat.model.viewRawMessage(model.id)
					}
					MenuItem {
						text: qsTr("Redact message")
					}
					MenuItem {
						visible: model.type == MtxEvent.ImageMessage || model.type == MtxEvent.VideoMessage || model.type == MtxEvent.AudioMessage || model.type == MtxEvent.FileMessage
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

		section {
			property: "section"
			delegate: Column {
				topPadding: 4
				bottomPadding: 4
				spacing: 8

				width: parent.width

				Label {
					id: dateBubble
					anchors.horizontalCenter: parent.horizontalCenter
					visible: section.includes(" ")
					text: chat.model.formatDateSeparator(new Date(Number(section.split(" ")[1])))
					color: colors.windowText

					height: contentHeight * 1.2
					width: contentWidth * 1.2
					horizontalAlignment: Text.AlignHCenter
					background: Rectangle {
						radius: parent.height / 2
						color: colors.dark
					}
				}
				Row {
					height: userName.height
					spacing: 4
					Avatar {
						width: avatarSize
						height: avatarSize
						url: chat.model.avatarUrl(section.split(" ")[0]).replace("mxc://", "image://MxcImage/")
						displayName: chat.model.displayName(section.split(" ")[0])
					}

					Text { 
						id: userName
						text: chat.model.escapeEmoji(chat.model.displayName(section.split(" ")[0]))
						color: chat.model.userColor(section.split(" ")[0], colors.window)
						textFormat: Text.RichText
					}
				}
			}
		}
	}
}
