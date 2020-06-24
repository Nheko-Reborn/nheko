import QtQuick 2.6

import im.nheko 1.0

Item {
	property double tempWidth: Math.min(parent ? parent.width : undefined, model.data.width < 1 ? parent.width : model.data.width)
	property double tempHeight: tempWidth * model.data.proportionalHeight

	property double divisor: model.isReply ? 4 : 2
	property bool tooHigh: tempHeight > timelineRoot.height / divisor

	height: tooHigh ? timelineRoot.height / divisor : tempHeight
	width: tooHigh ? (timelineRoot.height / divisor) / model.data.proportionalHeight : tempWidth

	Image {
		id: blurhash
		anchors.fill: parent
		visible: img.status != Image.Ready

		source: model.data.blurhash ? ("image://blurhash/" + model.data.blurhash) : ("image://colorimage/:/icons/icons/ui/do-not-disturb-rounded-sign@2x.png?"+colors.buttonText)
		asynchronous: true
		fillMode: Image.PreserveAspectFit

		sourceSize.width: parent.width
		sourceSize.height: parent.height
	}

	Image {
		id: img
		anchors.fill: parent

		source: model.data.url.replace("mxc://", "image://MxcImage/")
		asynchronous: true
		fillMode: Image.PreserveAspectFit

		MouseArea {
			enabled: model.data.type == MtxEvent.ImageMessage && img.status == Image.Ready
			anchors.fill: parent
			onClicked: TimelineManager.openImageOverlay(model.data.url, model.data.id)
		}
	}
}
