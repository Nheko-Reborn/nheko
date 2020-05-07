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

	FontMetrics {
		id: fontMetrics
	}

	Settings {
		id: settings
		category: "user"
		property bool avatar_circles: true
		property string emoji_font_family: "default"
	}

	Settings {
		id: timelineSettings
		category: "user/timeline"
		property bool buttons: true
	}

	Menu {
		id: messageContextMenu
		modal: true

		function show(eventId_, eventType_, isEncrypted_, showAt) {
			eventId = eventId_
			eventType = eventType_
			isEncrypted = isEncrypted_
			popup(showAt)
		}

		property string eventId
		property int eventType
		property bool isEncrypted

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
			visible: messageContextMenu.isEncrypted
			height: visible ? implicitHeight : 0
			text: qsTr("View decrypted raw message")
			onTriggered: chat.model.viewDecryptedRawMessage(messageContextMenu.eventId)
		}
		MenuItem {
			text: qsTr("Redact message")
			onTriggered: chat.model.redactEvent(messageContextMenu.eventId)
		}
		MenuItem {
			visible: messageContextMenu.eventType == MtxEvent.ImageMessage || messageContextMenu.eventType == MtxEvent.VideoMessage || messageContextMenu.eventType == MtxEvent.AudioMessage || messageContextMenu.eventType == MtxEvent.FileMessage || messageContextMenu.eventType == MtxEvent.Sticker
			height: visible ? implicitHeight : 0
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
			color: colors.text
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

			ScrollHelper {
				flickable: parent
				anchors.fill: parent
			}

			Shortcut {
				sequence: StandardKey.MoveToPreviousPage
				onActivated: { chat.contentY = chat.contentY - chat.height / 2; chat.returnToBounds(); }
			}
			Shortcut {
				sequence: StandardKey.MoveToNextPage
				onActivated: { chat.contentY = chat.contentY + chat.height / 2; chat.returnToBounds(); }
			}
			Shortcut {
				sequence: StandardKey.Cancel
				onActivated: chat.model.reply = undefined
			}
			Shortcut {
				sequence: "Alt+Up"
				onActivated: chat.model.reply = chat.model.indexToId(chat.model.reply? chat.model.idToIndex(chat.model.reply) + 1 : 0)
			}
			Shortcut {
				sequence: "Alt+Down"
				onActivated: {
					var idx = chat.model.reply? chat.model.idToIndex(chat.model.reply) - 1 : -1
					chat.model.reply = idx >= 0 ? chat.model.indexToId(idx) : undefined
				}
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
						color: colors.text

                        height: fontMetrics.height * 1.4
                        width: contentWidth * 1.2

                        horizontalAlignment: Text.AlignHCenter
						background: Rectangle {
							radius: parent.height / 2
							color: colors.base
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

            footer:  BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: chat.model && chat.model.paginationInProgress
                height: 50
                width: 50
                z: 3
            }
		}

		Rectangle {
			id: chatFooter

			height: Math.max(fontMetrics.height * 1.2, footerContent.height)
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

					color: colors.text
					text: chat.model ? chat.model.formatTypingUsers(chat.model.typingUsers, colors.window) : ""
					textFormat: Text.RichText
				}

				Rectangle {
					anchors.left: parent.left
					anchors.right: parent.right

					id: replyPopup

					visible: chat.model && chat.model.reply
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

						modelData: chat.model ? chat.model.getDump(chat.model.reply) : {}
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

						onClicked: chat.model.reply = undefined
					}
				}
			}
		}
	}
}
