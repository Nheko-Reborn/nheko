import QtQuick 2.9
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0
import QtQuick.Window 2.2

import im.nheko 1.0

import "./delegates"

Item {
	property var colors: currentActivePalette
	property var systemInactive: SystemPalette { colorGroup: SystemPalette.Disabled }
	property var inactiveColors: currentInactivePalette ? currentInactivePalette : systemInactive
	property int avatarSize: 40

	Rectangle {
		anchors.fill: parent
		color: colors.window

		Text {
			visible: !timelineManager.timeline && !timelineManager.isInitialSync
			anchors.centerIn: parent
			text: qsTr("No room open")
			font.pointSize: 24
			color: colors.windowText
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
						chat.forceLayout()
						chat.returnToBounds()
					}
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
							url: chat.model.avatarUrl(modelData.userId).replace("mxc://", "image://MxcImage/")
							displayName: modelData.userName

							MouseArea {
								anchors.fill: parent
								onClicked: chat.model.openUserProfile(modelData.userId)
								cursorShape: Qt.PointingHandCursor
							}
						}

						Text { 
							id: userName
							text: chat.model.escapeEmoji(modelData.userName)
							color: chat.model.userColor(modelData.userId, colors.window)
							textFormat: Text.RichText

							MouseArea {
								anchors.fill: parent
								onClicked: chat.model.openUserProfile(section.split(" ")[0])
								cursorShape: Qt.PointingHandCursor
							}
						}
					}
				}
			}

		}

		Rectangle {
			id: chatFooter

			height: Math.max(Math.max(16, typingDisplay.height), replyPopup.height)
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			z: 3

			color: colors.window

			Text {
				anchors.left: parent.left
				anchors.right: parent.right
				anchors.bottom: parent.bottom
				anchors.leftMargin: 10
				anchors.rightMargin: 10

				id: typingDisplay
				text: chat.model ? chat.model.formatTypingUsers(chat.model.typingUsers, chatFooter.color) : ""
				textFormat: Text.RichText
				color: colors.windowText
			}

			Rectangle {
				anchors.left: parent.left
				anchors.right: parent.right
				anchors.bottom: parent.bottom

				id: replyPopup

				visible: timelineManager.replyingEvent && chat.model
				width: parent.width
				// Height of child, plus margins, plus border
				height: replyContent.height + 10 + 1
				color: colors.window

				// For a border on the top.
				Rectangle {
					anchors.left: parent.left
				    anchors.right: parent.right
					height: 1
					width: parent.width
					color: colors.mid
				}
			
				RowLayout {
					id: replyContent
					anchors.left: parent.left
					anchors.right: parent.right
					anchors.top: parent.top
					anchors.margins: 10

					Column {
						Layout.fillWidth: true
						Layout.alignment: Qt.AlignTop
						spacing: 4
						Rectangle {
							width: parent.width
							height: replyContainer.height
							anchors.leftMargin: 10
							anchors.rightMargin: 10

							Rectangle {
								id: colorLine
								height: replyContainer.height
								width: 4
								color: chat.model ? chat.model.userColor(reply.modelData.userId, colors.window) : colors.window
							}

							Column {
								id: replyContainer
								anchors.left: colorLine.right
								anchors.leftMargin: 4
								width: parent.width - 8

								Text { 
									id: userName
									text: chat.model ? chat.model.escapeEmoji(reply.modelData.userName) : ""
									color: chat.model ? chat.model.userColor(reply.modelData.userId, colors.windowText) : colors.windowText
									textFormat: Text.RichText

									MouseArea {
										anchors.fill: parent
										onClicked: chat.model.openUserProfile(reply.modelData.userId)
										cursorShape: Qt.PointingHandCursor
									}
								}

								MessageDelegate {
									id: reply
									width: parent.width
									modelData: chat.model ? chat.model.getDump(timelineManager.replyingEvent) : {}
								}
							}

							color: { var col = chat.model ? chat.model.userColor(reply.modelData.userId, colors.window) : colors.window; col.a = 0.2; return col }

							MouseArea {
								anchors.fill: parent
								onClicked: chat.positionViewAtIndex(chat.model.idToIndex(timelineManager.replyingEvent), ListView.Contain)
								cursorShape: Qt.PointingHandCursor
							}
						}
					}
					ImageButton {
						Layout.alignment: Qt.AlignRight | Qt.AlignTop
						Layout.preferredHeight: 16
						id: closeReplyButton

						image: ":/icons/icons/ui/remove-symbol.png"
						ToolTip {
							visible: closeReplyButton.hovered
							text: qsTr("Close")
							palette: colors
						}

						onClicked: timelineManager.updateReplyingEvent(undefined)
					}
				}
			}
		}
	}
}
