import QtQuick 2.6

Row {
Rectangle {
	radius: 10
	color: colors.dark
	height: row.height
	width: row.width

	Row {
		id: row

		spacing: 15
		padding: 12

		Rectangle {
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
				onClicked: timelineManager.saveMedia(eventData.url, eventData.filename, eventData.mimetype, eventData.type)
				cursorShape: Qt.PointingHandCursor
			}
		}
		Column {
			TextEdit {
				text: eventData.body
				textFormat: TextEdit.PlainText
				readOnly: true
				wrapMode: Text.Wrap
				selectByMouse: true
				color: colors.text
			}
			TextEdit {
				text: eventData.filesize
				textFormat: TextEdit.PlainText
				readOnly: true
				wrapMode: Text.Wrap
				selectByMouse: true
				color: colors.text
			}
		}
	}
}
Rectangle {
}
}
