import QtQuick 2.6

Item {
	width: 300
	height: 300 * eventData.proportionalHeight

	Image {
		anchors.fill: parent

		source: eventData.url.replace("mxc://", "image://MxcImage/")
		asynchronous: true
		fillMode: Image.PreserveAspectFit
	}
}
