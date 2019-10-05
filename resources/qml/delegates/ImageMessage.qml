import QtQuick 2.6

import com.github.nheko 1.0

Item {
	width: 300
	height: 300 * eventData.proportionalHeight

	Image {
		id: img
		anchors.fill: parent

		source: eventData.url.replace("mxc://", "image://MxcImage/")
		asynchronous: true
		fillMode: Image.PreserveAspectFit

		MouseArea {
			enabled: eventData.type == MtxEvent.ImageMessage
			anchors.fill: parent
			onClicked: timelineManager.openImageOverlay(eventData.url, eventData.filename, eventData.mimetype, eventData.type)
		}
	}
}
