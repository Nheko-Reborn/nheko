import QtQuick 2.5
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.5

Rectangle {
	anchors.fill: parent

	Text {
		visible: !timelineManager.timeline
		anchors.centerIn: parent
		text: qsTr("No room open")
		font.pointSize: 24
	}

	ListView {
		visible: timelineManager.timeline != null
		anchors.fill: parent

		id: chat

		model: timelineManager.timeline
		delegate: RowLayout {
			width: chat.width
			Text {
				Layout.fillWidth: true
				height: contentHeight
				text: model.userName
			}

			Button {
				Layout.alignment: Qt.AlignRight
				id: replyButton
				flat: true
				height: replyButtonImg.contentHeight
				width: replyButtonImg.contentWidth
				ToolTip.visible: hovered
				ToolTip.text: qsTr("Reply")
				Image {
					id: replyButtonImg
					// Workaround, can't get icon.source working for now...
					anchors.fill: parent
					source: "qrc:/icons/icons/ui/mail-reply.png"
				}
			}
			Button {
				Layout.alignment: Qt.AlignRight
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
				Layout.alignment: Qt.AlignRight
				text: model.timestamp.toLocaleTimeString("HH:mm")
			}
		}

		section {
			property: "section"
			delegate: Column {
				width: parent.width
				Label {
					anchors.horizontalCenter: parent.horizontalCenter
					visible: section.includes(" ")
					text: Qt.formatDate(new Date(Number(section.split(" ")[1])))
					height: contentHeight * 1.2
					width: contentWidth * 1.2
					horizontalAlignment: Text.AlignHCenter
					background: Rectangle {
						radius: parent.height / 2
						color: "black"
					}
				}
				Text { text: section.split(" ")[0] }
			}
		}
	}
}
