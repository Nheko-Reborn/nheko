import QtQuick 2.3
import QtQuick.Controls 2.3

AbstractButton {
	property string image
	property string src
	width: 16
	height: 16
	id: button

	Image {
		id: buttonImg
		// Workaround, can't get icon.source working for now...
		anchors.fill: parent
		source: src ? src : ("image://colorimage/" + image + "?" + (button.hovered ? colors.highlight : colors.buttonText))
	}

	MouseArea
	{
		id: mouseArea
		anchors.fill: parent
		onPressed:  mouse.accepted = false
		cursorShape: Qt.PointingHandCursor
	}
}
