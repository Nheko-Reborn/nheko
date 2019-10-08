import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0
import QtQuick.Window 2.2
import Qt.labs.qmlmodels 1.0

import com.github.nheko 1.0

import "./delegates"

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
		delegate: DelegateChooser {
			role: "type"
			DelegateChoice {
				roleValue: MtxEvent.TextMessage
				TimelineRow { view: chat; TextMessage { id: kid } }
			}
			DelegateChoice {
				roleValue: MtxEvent.NoticeMessage
				TimelineRow { view: chat; NoticeMessage { id: kid } }
			}
			DelegateChoice {
				roleValue: MtxEvent.EmoteMessage
				TimelineRow { view: chat; TextMessage { id: kid } }
			}
			DelegateChoice {
				roleValue: MtxEvent.ImageMessage
				TimelineRow { view: chat; ImageMessage { id: kid } }
			}
			DelegateChoice {
				roleValue: MtxEvent.Sticker
				TimelineRow { view: chat; ImageMessage { id: kid } }
			}
			DelegateChoice {
				roleValue: MtxEvent.FileMessage
				TimelineRow { view: chat; FileMessage { id: kid } }
			}
			DelegateChoice {
				roleValue: MtxEvent.VideoMessage
				TimelineRow { view: chat; PlayableMediaMessage { id: kid } }
			}
			DelegateChoice {
				roleValue: MtxEvent.AudioMessage
				TimelineRow { view: chat; PlayableMediaMessage { id: kid } }
			}
			DelegateChoice {
				roleValue: MtxEvent.Redacted
				TimelineRow { view: chat; Redacted { id: kid } }
			}
			DelegateChoice {
				//roleValue: MtxEvent.Redacted
				TimelineRow { view: chat; Placeholder { id: kid } }
			}
		}


		section {
			property: "section"
			delegate: Column {
				topPadding: 4
				bottomPadding: 4
				spacing: 8

				width: parent.width

				Component.onCompleted: chat.forceLayout()

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
