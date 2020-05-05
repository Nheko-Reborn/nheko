import QtQuick 2.6
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0

Rectangle {
	id: avatar
	width: 48
	height: 48
	radius: settings.avatar_circles ? height/2 : 3

	property alias url: img.source
	property string displayName

	Label {
		anchors.fill: parent
		text: chat.model.escapeEmoji(String.fromCodePoint(displayName.codePointAt(0)))
		textFormat: Text.RichText
		font.pixelSize: avatar.height/2
		verticalAlignment: Text.AlignVCenter
		horizontalAlignment: Text.AlignHCenter
		visible: img.status != Image.Ready
		color: colors.text
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
	color: colors.base
}
