import QtQuick 2.6
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.1
import QtMultimedia 5.6

import com.github.nheko 1.0

Rectangle {
	id: bg
	radius: 10
	color: colors.dark
	height: content.height + 24
	width: parent.width

	Column { 
		id: content
		width: parent.width - 24
		anchors.centerIn: parent

		Rectangle {
			id: videoContainer
			visible: model.type == MtxEvent.VideoMessage
			width: Math.min(parent.width, model.width)
			height: width*model.proportionalHeight
			Image {
				anchors.fill: parent
				source: model.thumbnailUrl.replace("mxc://", "image://MxcImage/")
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
				color: colors.light
				radius: 22
				height: 44
				width: 44
				Image {
					id: img
					anchors.centerIn: parent

					source: "qrc:/icons/icons/ui/arrow-pointing-down.png"
					fillMode: Image.Pad

				}
				MouseArea {
					anchors.fill: parent
					onClicked: {
						switch (button.state) {
							case "": timelineManager.cacheMedia(model.url, model.mimetype); break;
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
					target: timelineManager
					onMediaCached: {
						if (mxcUrl == model.url) {
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
						PropertyChanges { target: img; source: "qrc:/icons/icons/ui/play-sign.png" }
					},
					State {
						name: "playing"
						PropertyChanges { target: img; source: "qrc:/icons/icons/ui/pause-symbol.png" }
					}
				]
			}
			ColumnLayout {
				id: col

				Text {
					Layout.fillWidth: true
					text: model.body
					textFormat: Text.PlainText
					elide: Text.ElideRight
					color: colors.text
				}
				Text {
					Layout.fillWidth: true
					text: model.filesize
					textFormat: Text.PlainText
					elide: Text.ElideRight
					color: colors.text
				}
			}
		}
	}
}

