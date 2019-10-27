import QtQuick 2.6

import com.github.nheko 1.0

Item {
	width: Math.min(parent ? parent.width : undefined, model.width)
	height: width * model.proportionalHeight

	Image {
		id: img
		anchors.fill: parent

		source: model.url.replace("mxc://", "image://MxcImage/")
		asynchronous: true
		fillMode: Image.PreserveAspectFit

		MouseArea {
			enabled: model.type == MtxEvent.ImageMessage
			anchors.fill: parent
			onClicked: timelineManager.openImageOverlay(model.url, model.filename, model.mimetype, model.type)
		}
	}
}
