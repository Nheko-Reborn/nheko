import QtQuick 2.6

import im.nheko 1.0

Item {
	property double tempWidth: Math.min(parent ? parent.width : undefined, model.data.width)
	property double tempHeight: tempWidth * model.data.proportionalHeight

	property bool tooHigh: tempHeight > timelineRoot.height / 2

	height: tooHigh ? timelineRoot.height / 2 : tempHeight
	width: tooHigh ? (timelineRoot.height / 2) / model.data.proportionalHeight : tempWidth

	Image {
		id: img
		anchors.fill: parent

		source: model.data.url.replace("mxc://", "image://MxcImage/")
		asynchronous: true
		fillMode: Image.PreserveAspectFit

		MouseArea {
			enabled: model.data.type == MtxEvent.ImageMessage
			anchors.fill: parent
			onClicked: timelineManager.openImageOverlay(model.data.url, model.data.id)
		}
	}
}
