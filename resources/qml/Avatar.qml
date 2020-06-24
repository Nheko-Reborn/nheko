import QtQuick 2.6
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0

import im.nheko 1.0

Rectangle {
	id: avatar
	width: 48
	height: 48
	radius: Settings.avatarCircles ? height/2 : 3

	property alias url: img.source
	property string userid
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
				radius: Settings.avatarCircles ? height/2 : 3
			}
		}

	}

	Rectangle {
		anchors.bottom: avatar.bottom
		anchors.right: avatar.right

		height: avatar.height / 6
		width: height
		radius: Settings.avatarCircles ? height / 2 : height / 4
		color: switch (TimelineManager.userPresence(userid)) {
			case "online": return "#00cc66"
			case "unavailable": return "#ff9933"
			case "offline": // return "#a82353" don't show anything if offline, since it is confusing, if presence is disabled
			default: "transparent"
		}
	}

	color: colors.base
}
