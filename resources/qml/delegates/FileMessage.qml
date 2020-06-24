import QtQuick 2.6
import QtQuick.Layouts 1.2

import im.nheko 1.0

Item {
	height: row.height + 24
	width: parent ? parent.width : undefined

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
				onClicked: TimelineManager.timeline.saveMedia(model.data.id)
				cursorShape: Qt.PointingHandCursor
			}
		}
		ColumnLayout {
			id: col

			Text {
				id: filename
				Layout.fillWidth: true
				text: model.data.body
				textFormat: Text.PlainText
				elide: Text.ElideRight
				color: colors.text
			}
			Text {
				id: filesize
				Layout.fillWidth: true
				text: model.data.filesize
				textFormat: Text.PlainText
				elide: Text.ElideRight
				color: colors.text
			}
		}
	}

	Rectangle {
		color: colors.dark
		z: -1
		radius: 10
		height: row.height + 24
		width: 44 + 24 + 24 + Math.max(Math.min(filesize.width, filesize.implicitWidth), Math.min(filename.width, filename.implicitWidth))
	}

}
