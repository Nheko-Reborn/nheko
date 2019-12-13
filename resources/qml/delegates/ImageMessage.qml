import QtQuick 2.6

import im.nheko 1.0

Item {
	property double tempWidth: Math.min(parent ? parent.width : undefined, model.width)
	property double tempHeight: tempWidth * model.proportionalHeight

	property bool tooHigh: tempHeight > chat.height - 40

	height: tooHigh ? chat.height - 40 : tempHeight
	width: tooHigh ? (chat.height - 40) / model.proportionalHeight : tempWidth

	Image {
		id: img
		anchors.fill: parent

		source: model.url.replace("mxc://", "image://MxcImage/")
		asynchronous: true
		fillMode: Image.PreserveAspectFit

		MouseArea {
			enabled: model.type == MtxEvent.ImageMessage
			anchors.fill: parent
			onClicked: timelineManager.openImageOverlay(model.url, model.id)
		}
	}
}
