import QtQuick 2.1

Rectangle {
	anchors.fill: parent

	Text {
		visible: !timelineManager.timeline
		anchors.centerIn: parent
		text: qsTr("No room open")
		font.pointSize: 24
	}
	Text {
		visible: timelineManager.timeline != null
		anchors.centerIn: parent
		text: qsTr("room open")
		font.pointSize: 24
	}

	ListView {
		visible: timelineManager.timeline != null
		anchors.fill: parent

		id: chat

		model: timelineManager.timeline
		delegate: Text {
			height: contentHeight
			text: model.userId
		}
	}
}
