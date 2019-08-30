import QtQuick 2.1

Rectangle {
	anchors.fill: parent

	Text {
		visible: !timeline
		anchors.centerIn: parent
		text: qsTr("No room open")
		font.pointSize: 24
	}

	ListView {
		visible: timeline != undefined
		anchors.fill: parent

		model: timeline
		delegate: Text {
			text: userId
		}
					}
}
