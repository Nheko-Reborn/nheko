import QtQuick 2.6
import QtQuick.Layouts 1.6
import QtMultimedia 5.12

Rectangle {
	radius: 10
	color: colors.dark
	height: row.height + 24
	width: parent.width

	RowLayout {
		id: row

		anchors.centerIn: parent
		width: parent.width - 24

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
						case "": timelineManager.cacheMedia(eventData.url, eventData.mimetype); break;
						case "stopped":
							audio.play(); console.log("play");
							button.state = "playing"
							break
						case "playing":
							audio.pause(); console.log("pause");
							button.state = "stopped"
							break
					}
				}
				cursorShape: Qt.PointingHandCursor
			}
			MediaPlayer {
				id: audio
				onError: console.log(errorString)
			}

			Connections {
				target: timelineManager
				onMediaCached: {
					if (mxcUrl == eventData.url) {
						audio.source = "file://" + cacheUrl
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
				text: eventData.body
				textFormat: Text.PlainText
				elide: Text.ElideRight
				color: colors.text
			}
			Text {
				Layout.fillWidth: true
				text: eventData.filesize
				textFormat: Text.PlainText
				elide: Text.ElideRight
				color: colors.text
			}
		}
	}
}

