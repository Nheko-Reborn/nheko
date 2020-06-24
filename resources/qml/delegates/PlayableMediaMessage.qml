import QtQuick 2.6
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.1
import QtMultimedia 5.6

import im.nheko 1.0

Rectangle {
	id: bg
	radius: 10
	color: colors.dark
	height: content.height + 24
	width: parent ? parent.width : undefined

	Column { 
		id: content
		width: parent.width - 24
		anchors.centerIn: parent

		Rectangle {
			id: videoContainer
			visible: model.data.type == MtxEvent.VideoMessage
			property double tempWidth: Math.min(parent ? parent.width : undefined, model.data.width < 1 ? 400 : model.data.width)
			property double tempHeight: tempWidth * model.data.proportionalHeight

			property double divisor: model.isReply ? 4 : 2
			property bool tooHigh: tempHeight > timelineRoot.height / divisor

			height: tooHigh ? timelineRoot.height / divisor : tempHeight
			width: tooHigh ? (timelineRoot.height / divisor) / model.data.proportionalHeight : tempWidth
			Image {
				anchors.fill: parent
				source: model.data.thumbnailUrl.replace("mxc://", "image://MxcImage/")
				asynchronous: true
				fillMode: Image.PreserveAspectFit

				VideoOutput {
					anchors.fill: parent
					fillMode: VideoOutput.PreserveAspectFit
					source: media
				}
			}
		}

		RowLayout {
			width: parent.width
			Text {
				id: positionText
				text: "--:--:--"
				color: colors.text
			}
			Slider {
				Layout.fillWidth: true
				id: progress
				value: media.position
				from: 0
				to: media.duration

				onMoved: media.seek(value)
				//indeterminate: true
				function updatePositionTexts() {
					function formatTime(date) {
						var hh = date.getUTCHours();
						var mm = date.getUTCMinutes();
						var ss = date.getSeconds();
						if (hh < 10) {hh = "0"+hh;}
						if (mm < 10) {mm = "0"+mm;}
						if (ss < 10) {ss = "0"+ss;}
						return hh+":"+mm+":"+ss;
					}
					positionText.text = formatTime(new Date(media.position))
					durationText.text = formatTime(new Date(media.duration))
				}
				onValueChanged: updatePositionTexts()

				palette: colors
			}
			Text {
				id: durationText
				text: "--:--:--"
				color: colors.text
			}
		}

		RowLayout {
			width: parent.width

			spacing: 15

			Rectangle {
				id: button
				color: colors.window
				radius: 22
				height: 44
				width: 44
				Image {
					id: img
					anchors.centerIn: parent
					z: 3

					source: "image://colorimage/:/icons/icons/ui/arrow-pointing-down.png?"+colors.text
					fillMode: Image.Pad

				}
				MouseArea {
					anchors.fill: parent
					onClicked: {
						switch (button.state) {
							case "": TimelineManager.timeline.cacheMedia(model.data.id); break;
							case "stopped":
							media.play(); console.log("play");
							button.state = "playing"
							break
							case "playing":
							media.pause(); console.log("pause");
							button.state = "stopped"
							break
						}
					}
					cursorShape: Qt.PointingHandCursor
				}
				MediaPlayer {
					id: media
					onError: console.log(errorString)
					onStatusChanged: if(status == MediaPlayer.Loaded) progress.updatePositionTexts()
					onStopped: button.state = "stopped"
				}

				Connections {
					target: TimelineManager.timeline
					onMediaCached: {
						if (mxcUrl == model.data.url) {
							media.source = "file://" + cacheUrl
							button.state = "stopped"
							console.log("media loaded: " + mxcUrl + " at " + cacheUrl)
						}
						console.log("media cached: " + mxcUrl + " at " + cacheUrl)
					}
				}

				states: [
					State {
						name: "stopped"
						PropertyChanges { target: img; source: "image://colorimage/:/icons/icons/ui/play-sign.png?"+colors.text }
					},
					State {
						name: "playing"
						PropertyChanges { target: img; source: "image://colorimage/:/icons/icons/ui/pause-symbol.png?"+colors.text }
					}
				]
			}
			ColumnLayout {
				id: col

				Text {
					Layout.fillWidth: true
					text: model.data.body
					textFormat: Text.PlainText
					elide: Text.ElideRight
					color: colors.text
				}
				Text {
					Layout.fillWidth: true
					text: model.data.filesize
					textFormat: Text.PlainText
					elide: Text.ElideRight
					color: colors.text
				}
			}
		}
	}
}

