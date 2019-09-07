import QtQuick 2.6
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.5
import QtGraphicalEffects 1.0

import com.github.nheko 1.0

Rectangle {
	anchors.fill: parent

	SystemPalette { id: colors; colorGroup: SystemPalette.Active }
	SystemPalette { id: inactiveColors; colorGroup: SystemPalette.Disabled }
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

		visible: timelineManager.timeline != null
		anchors.fill: parent

		ScrollBar.vertical: ScrollBar {
			id: scrollbar
			anchors.top: parent.top
			anchors.right: parent.right
			anchors.bottom: parent.bottom
		}

		model: timelineManager.timeline
		spacing: 4
		delegate: RowLayout {
			anchors.leftMargin: 52
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.rightMargin: scrollbar.width

			Loader {
				id: loader
				Layout.fillWidth: true
				height: item.height
				Layout.alignment: Qt.AlignTop

				source: switch(model.type) {
					case MtxEvent.Aliases: return "delegates/Aliases.qml"
					case MtxEvent.Avatar: return "delegates/Avatar.qml"
					case MtxEvent.CanonicalAlias: return "delegates/CanonicalAlias.qml"
					case MtxEvent.Create: return "delegates/Create.qml"
					case MtxEvent.GuestAccess: return "delegates/GuestAccess.qml"
					case MtxEvent.HistoryVisibility: return "delegates/HistoryVisibility.qml"
					case MtxEvent.JoinRules: return "delegates/JoinRules.qml"
					case MtxEvent.Member: return "delegates/Member.qml"
					case MtxEvent.Name: return "delegates/Name.qml"
					case MtxEvent.PowerLevels: return "delegates/PowerLevels.qml"
					case MtxEvent.Topic: return "delegates/Topic.qml"
					case MtxEvent.NoticeMessage: return "delegates/NoticeMessage.qml"
					case MtxEvent.TextMessage: return "delegates/TextMessage.qml"
					case MtxEvent.ImageMessage: return "delegates/ImageMessage.qml"
					case MtxEvent.VideoMessage: return "delegates/VideoMessage.qml"
					default: return "delegates/placeholder.qml"
				}
				property variant eventData: model
			}


			Button {
				Layout.alignment: Qt.AlignRight | Qt.AlignTop
				id: replyButton
				flat: true
				height: 32
				width: 32
				ToolTip.visible: hovered
				ToolTip.text: qsTr("Reply")

				Image {
					id: replyButtonImg
					// Workaround, can't get icon.source working for now...
					anchors.fill: parent
					source: "qrc:/icons/icons/ui/mail-reply.png"
				}
				ColorOverlay {
					anchors.fill: replyButtonImg
					source: replyButtonImg
					color: colors.buttonText
				}
			}
			Button {
				Layout.alignment: Qt.AlignRight | Qt.AlignTop
				id: optionsButton
				flat: true
				height: optionsButtonImg.contentHeight
				width: optionsButtonImg.contentWidth
				ToolTip.visible: hovered
				ToolTip.text: qsTr("Options")
				Image {
					id: optionsButtonImg
					// Workaround, can't get icon.source working for now...
					anchors.fill: parent
					source: "qrc:/icons/icons/ui/vertical-ellipsis.png"
				}
				ColorOverlay {
					anchors.fill: optionsButtonImg
					source: optionsButtonImg
					color: colors.buttonText
				}

				onClicked: contextMenu.open()

				Menu {
					y: optionsButton.height
					id: contextMenu

					MenuItem {
						text: "Read receipts"
					}
					MenuItem {
						text: "Mark as read"
					}
					MenuItem {
						text: "View raw message"
					}
					MenuItem {
						text: "Redact message"
					}
				}
			}

			Text {
				Layout.alignment: Qt.AlignRight | Qt.AlignTop
				text: model.timestamp.toLocaleTimeString("HH:mm")
				color: inactiveColors.text
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
					Rectangle {
						width: 48
						height: 48
						color: "green"
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
