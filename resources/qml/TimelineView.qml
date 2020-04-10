import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0
import QtQuick.Window 2.2
import Qt.labs.settings 1.0

import im.nheko 1.0

import "./delegates"

Page {
	property var colors: currentActivePalette
	property var systemInactive: SystemPalette { colorGroup: SystemPalette.Disabled }
	property var inactiveColors: currentInactivePalette ? currentInactivePalette : systemInactive
	property int avatarSize: 40

	palette: colors

	Settings {
		id: settings
		category: "user"
		property bool avatar_circles: true
	}

	Settings {
		id: timelineSettings
		category: "user/timeline"
		property bool buttons: true
	}

	Menu {
		id: messageContextMenu
		modal: true

		function show(eventId_, eventType_, showAt) {
			eventId = eventId_
			eventType = eventType_
			popup(showAt)
		}

		property string eventId
		property int eventType

		MenuItem {
			text: qsTr("Reply")
			onClicked: chat.model.replyAction(messageContextMenu.eventId)
		}
		MenuItem {
			text: qsTr("Read receipts")
			onTriggered: chat.model.readReceiptsAction(messageContextMenu.eventId)
		}
		MenuItem {
			text: qsTr("Mark as read")
		}
		MenuItem {
			text: qsTr("View raw message")
			onTriggered: chat.model.viewRawMessage(messageContextMenu.eventId)
		}
		MenuItem {
			text: qsTr("Redact message")
			onTriggered: chat.model.redactEvent(messageContextMenu.eventId)
		}
		MenuItem {
			visible: messageContextMenu.eventType == MtxEvent.ImageMessage || messageContextMenu.eventType == MtxEvent.VideoMessage || messageContextMenu.eventType == MtxEvent.AudioMessage || messageContextMenu.eventType == MtxEvent.FileMessage || messageContextMenu.eventType == MtxEvent.Sticker
			text: qsTr("Save as")
			onTriggered: timelineManager.timeline.saveMedia(messageContextMenu.eventId)
		}
	}

	id: timelineRoot

	Rectangle {
		anchors.fill: parent
		color: colors.window

		Label {
			visible: !timelineManager.timeline && !timelineManager.isInitialSync
			anchors.centerIn: parent
			text: qsTr("No room open")
			font.pointSize: 24
		}

		BusyIndicator {
			anchors.centerIn: parent
			running: timelineManager.isInitialSync
			height: 200
			width: 200
			z: 3
		}

		ListView {
			id: chat

			visible: timelineManager.timeline != null

			anchors.left: parent.left
			anchors.right: parent.right
			anchors.top: parent.top
			anchors.bottom: chatFooter.top

			anchors.leftMargin: 4
			anchors.rightMargin: scrollbar.width

			model: timelineManager.timeline

			boundsBehavior: Flickable.StopAtBounds
			pixelAligned: true

			MouseArea {
				anchors.fill: parent
				acceptedButtons: Qt.NoButton
				propagateComposedEvents: true
				z: -1
				onWheel: {
					if (wheel.angleDelta != 0) {
						chat.contentY = chat.contentY - wheel.angleDelta.y
						wheel.accepted = true
						chat.returnToBounds()
					}
				}
			}

			Shortcut {
				sequence: StandardKey.MoveToPreviousPage
				onActivated: { chat.contentY = chat.contentY - chat.height / 2; chat.returnToBounds(); }
			}
			Shortcut {
				sequence: StandardKey.MoveToNextPage
				onActivated: { chat.contentY = chat.contentY + chat.height / 2; chat.returnToBounds(); }
			}

			ScrollBar.vertical: ScrollBar {
				id: scrollbar
				parent: chat.parent
				anchors.top: chat.top
				anchors.left: chat.right
				anchors.bottom: chat.bottom
			}

			spacing: 4
			verticalLayoutDirection: ListView.BottomToTop

			onCountChanged: if (atYEnd) model.currentIndex = 0 // Mark last event as read, since we are at the bottom

			delegate: Rectangle {
				// This would normally be previousSection, but our model's order is inverted.
				property bool sectionBoundary: (ListView.nextSection != "" && ListView.nextSection !== ListView.section) || model.index === chat.count - 1

				id: wrapper
				property Item section
				width: chat.width
				height: section ? section.height + timelinerow.height : timelinerow.height
				color: "transparent"

				TimelineRow {
					id: timelinerow
					y: section ? section.y + section.height : 0
				}

				onSectionBoundaryChanged: {
					if (sectionBoundary) {
						var properties = {
							'modelData': model.dump,
							'section': ListView.section,
							'nextSection': ListView.nextSection
						}
						section = sectionHeader.createObject(wrapper, properties)
					} else {
						section.destroy()
						section = null
					}
				}

				Binding {
					target: chat.model
					property: "currentIndex"
					when: y + height + 2 * chat.spacing > chat.contentY + chat.height && y < chat.contentY + chat.height
					value: index
					delayed: true
				}

			}

			section {
				property: "section"
			}
			Component {
				id: sectionHeader
				Column {
					property var modelData
					property string section
					property string nextSection

					topPadding: 4
					bottomPadding: 4
					spacing: 8

					visible: !!modelData

					width: parent.width
					height: (section.includes(" ") ? dateBubble.height + 8 + userName.height : userName.height) + 8

					Label {
						id: dateBubble
						anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
						visible: section.includes(" ")
						text: chat.model.formatDateSeparator(modelData.timestamp)
						color: colors.brightText

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
							url: chat.model.avatarUrl(modelData.userId).replace("mxc://", "image://MxcImage/")
							displayName: modelData.userName

							MouseArea {
								anchors.fill: parent
								onClicked: chat.model.openUserProfile(modelData.userId)
								cursorShape: Qt.PointingHandCursor
								propagateComposedEvents: true
							}
						}

						Label { 
							id: userName
							text: chat.model.escapeEmoji(modelData.userName)
							color: timelineManager.userColor(modelData.userId, colors.window)
							textFormat: Text.RichText

							MouseArea {
								anchors.fill: parent
								onClicked: chat.model.openUserProfile(section.split(" ")[0])
								cursorShape: Qt.PointingHandCursor
								propagateComposedEvents: true
							}
						}
					}
				}
			}

		}

		Rectangle {
			id: chatFooter

			height: Math.max(16, footerContent.height)
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			z: 3

			color: "transparent"

			Column {
				id: footerContent
				anchors.left: parent.left
				anchors.right: parent.right

				Label {
					id: typingDisplay
					anchors.left: parent.left
					anchors.right: parent.right
					anchors.leftMargin: 10
					anchors.rightMargin: 10

					text: chat.model ? chat.model.formatTypingUsers(chat.model.typingUsers, colors.window) : ""
					textFormat: Text.RichText
				}

				Rectangle {
					anchors.left: parent.left
					anchors.right: parent.right

					id: replyPopup

					visible: timelineManager.replyingEvent && chat.model
					// Height of child, plus margins, plus border
					height: replyPreview.height + 10
					color: colors.base


					Reply {
						id: replyPreview

						anchors.left: parent.left
						anchors.leftMargin: 10
						anchors.right: closeReplyButton.left
						anchors.rightMargin: 20
						anchors.bottom: parent.bottom

						modelData: chat.model ? chat.model.getDump(timelineManager.replyingEvent) : {}
						userColor: timelineManager.userColor(modelData.userId, colors.window)
					}

					ImageButton {
						id: closeReplyButton

						anchors.right: parent.right
						anchors.rightMargin: 15
						anchors.top: replyPreview.top
						hoverEnabled: true
						width: 16
						height: 16

						image: ":/icons/icons/ui/remove-symbol.png"
						ToolTip.visible: closeReplyButton.hovered
						ToolTip.text: qsTr("Close")

						onClicked: timelineManager.closeReply()
					}
				}
			}
		}
	}
}
