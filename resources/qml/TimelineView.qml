import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0
import QtQuick.Window 2.2

import im.nheko 1.0
import im.nheko.EmojiModel 1.0

import "./delegates"
import "./emoji"
import "./device-verification"

Page {
	id: timelineRoot

	property var colors: currentActivePalette
	property var systemInactive: SystemPalette { colorGroup: SystemPalette.Disabled }
	property var inactiveColors: currentInactivePalette ? currentInactivePalette : systemInactive
	property int avatarSize: 40
	property real highlightHue: colors.highlight.hslHue
	property real highlightSat: colors.highlight.hslSaturation
	property real highlightLight: colors.highlight.hslLightness
	property variant userProfile

	palette: colors

	FontMetrics {
		id: fontMetrics
	}

	EmojiPicker {
		id: emojiPopup
		width: 7 * 52 + 20
		height: 6 * 52 
		colors: palette
		model: EmojiProxyModel {
			category: EmojiCategory.People
			sourceModel: EmojiModel {}
		}
	}

	Menu {
		id: messageContextMenu
		modal: true

		function show(eventId_, eventType_, isEncrypted_, showAt_, position) {
			eventId = eventId_
			eventType = eventType_
			isEncrypted = isEncrypted_

			if (position)
			popup(position, showAt_)
			else
			popup(showAt_)
		}

		property string eventId
		property int eventType
		property bool isEncrypted

		MenuItem {
			text: qsTr("React")
			onClicked: emojiPopup.show(messageContextMenu.parent, messageContextMenu.eventId)
		}
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
			onTriggered: TimelineManager.timeline.saveMedia(messageContextMenu.eventId)
		}
	}

	Rectangle {
		anchors.fill: parent
		color: colors.window

		Component {
			id: deviceVerificationDialog
			DeviceVerification {}
		}
		Connections {
			target: TimelineManager
			function onNewDeviceVerificationRequest(flow,transactionId,userId,deviceId,isRequest) {
				flow.userId = userId;
				flow.sender = false;
				flow.deviceId = deviceId;
				switch(flow.type){
					case DeviceVerificationFlow.ToDevice:
					    flow.tranId = transactionId;
						deviceVerificationList.add(flow.tranId);
						break;
					case DeviceVerificationFlow.RoomMsg:
						deviceVerificationList.add(flow.tranId);
						break;
				}
				var dialog = deviceVerificationDialog.createObject(timelineRoot, {flow: flow,isRequest = isRequest});
				dialog.show();
			}
		}
		Connections {
			target: TimelineManager.timeline
			function onOpenProfile(profile) {
				var userProfile = userProfileComponent.createObject(timelineRoot,{profile: profile});
				userProfile.show();
			}
		}

		Label {
			visible: !TimelineManager.timeline && !TimelineManager.isInitialSync
			anchors.centerIn: parent
			text: qsTr("No room open")
			font.pointSize: 24
			color: colors.text
		}

		BusyIndicator {
			visible: running
			anchors.centerIn: parent
			running: TimelineManager.isInitialSync
			height: 200
			width: 200
			z: 3
		}

		ListView {
			id: chat

			visible: TimelineManager.timeline != null

			cacheBuffer: 400

			anchors.horizontalCenter: parent.horizontalCenter
			anchors.top: parent.top
			anchors.bottom: chatFooter.top
			width: parent.width

			anchors.leftMargin: 4
			anchors.rightMargin: scrollbar.width

			model: TimelineManager.timeline

			boundsBehavior: Flickable.StopAtBounds

			ScrollHelper {
				flickable: parent
				anchors.fill: parent
			}

			pixelAligned: true

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
				anchors.right: chat.right
				anchors.bottom: chat.bottom
			}

			spacing: 4
			verticalLayoutDirection: ListView.BottomToTop

			onCountChanged: if (atYEnd) model.currentIndex = 0 // Mark last event as read, since we are at the bottom

			property int delegateMaxWidth: (Settings.timelineMaxWidth > 100 && (parent.width - Settings.timelineMaxWidth) > 32) ? Settings.timelineMaxWidth : (parent.width - 32)

			delegate: Rectangle {
				// This would normally be previousSection, but our model's order is inverted.
				property bool sectionBoundary: (ListView.nextSection != "" && ListView.nextSection !== ListView.section) || model.index === chat.count - 1

				id: wrapper
				property Item section
				anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
				width: chat.delegateMaxWidth
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

				Connections {
					target: chat
					function onMovementEnded() {
						if (y + height + 2 * chat.spacing > chat.contentY + chat.height && y < chat.contentY + chat.height)
							chat.model.currentIndex = index;
					}
				}
			}

			Component{
				id: userProfileComponent
				UserProfile{}
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
						spacing: 8

						Avatar {
							width: avatarSize
							height: avatarSize
							url: chat.model.avatarUrl(modelData.userId).replace("mxc://", "image://MxcImage/")
							displayName: modelData.userName
							userid: modelData.userId

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
							color: TimelineManager.userColor(modelData.userId, colors.window)
							textFormat: Text.RichText

							MouseArea {
								anchors.fill: parent
								Layout.alignment: Qt.AlignHCenter
								onClicked: chat.model.openUserProfile(modelData.userId)
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

<<<<<<< HEAD
						modelData: chat.model ? chat.model.getDump(chat.model.reply, chat.model.id) : {}
						userColor: timelineManager.userColor(modelData.userId, colors.window)
=======
						modelData: chat.model ? chat.model.getDump(chat.model.reply) : {}
						userColor: TimelineManager.userColor(modelData.userId, colors.window)
>>>>>>> Fix presence indicator
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
