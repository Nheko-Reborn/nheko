import QtQuick 2.3
import QtQuick.Layouts 1.10

Rectangle {
	color: "red"
	implicitHeight: Qt.application.font.pixelSize * 4
	implicitWidth: col.width
	height: Qt.application.font.pixelSize * 4
	width: col.width
	ColumnLayout {
		id: col
		anchors.bottom: parent.bottom
		property var emoji: emojis.mapping[Math.floor(Math.random()*64)]
		Label {
			height: font.pixelSize * 2
			Layout.alignment: Qt.AlignHCenter
			text: col.emoji.emoji
			font.pixelSize: Qt.application.font.pixelSize * 2
		}
		Label {
			Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
			text: col.emoji.description
		}
	}
}
