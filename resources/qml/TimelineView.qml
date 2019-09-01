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
		delegate: RowLayout {
			anchors.leftMargin: 52
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.rightMargin: scrollbar.width

			Text {
				Layout.fillWidth: true
				height: contentHeight
				text: "Event content"
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
				height: dateBubble.visible ? dateBubble.height + userName.height : userName.height
				Label {
					id: dateBubble
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
				Row {
					spacing: 4
					Rectangle {
						width: 48
						height: 48
						color: "green"
					}

					Text { 
						id: userName
						text: chat.model.displayName(section.split(" ")[0])
						color: chat.model.userColor(section.split(" ")[0], "#ffffff")
					}
				}
			}
		}
	}
}
