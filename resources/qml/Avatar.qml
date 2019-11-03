import QtQuick 2.6
import QtGraphicalEffects 1.0
import Qt.labs.settings 1.0

Rectangle {
	id: avatar
	width: 48
	height: 48
	radius: settings.avatar_circles ? height/2 : 3

	Settings {
		id: settings
		category: "user"
		property bool avatar_circles: true
	}

	property alias url: img.source
	property string displayName

	Text {
		anchors.fill: parent
		text: String.fromCodePoint(displayName.codePointAt(0))
		color: colors.text
		font.pixelSize: avatar.height/2
		verticalAlignment: Text.AlignVCenter
		horizontalAlignment: Text.AlignHCenter
	}

	Image {
		id: img
		anchors.fill: parent
		asynchronous: true
		fillMode: Image.PreserveAspectCrop
		mipmap: true
		smooth: false

		sourceSize.width: avatar.width
		sourceSize.height: avatar.height

		layer.enabled: true
		layer.effect: OpacityMask {
			maskSource: Rectangle {
				anchors.fill: parent
				width: avatar.width
				height: avatar.height
				radius: settings.avatar_circles ? height/2 : 3
			}
		}
	}
	color: colors.dark
}
