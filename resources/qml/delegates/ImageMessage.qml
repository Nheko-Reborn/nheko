import QtQuick 2.6

import im.nheko 1.0

Item {
	property double tempWidth: Math.min(parent ? parent.width : undefined, model.data.width)
	property double tempHeight: tempWidth * model.data.proportionalHeight

	property bool tooHigh: tempHeight > chat.height - 40

	height: tooHigh ? chat.height - 40 : tempHeight
	width: tooHigh ? (chat.height - 40) / model.data.proportionalHeight : tempWidth

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
