import QtQuick 2.3
import QtQuick.Controls 2.3

Button {
	property string image: undefined

	id: button

	flat: true

	// disable background, because we don't want a border on hover
	background: Item {
	}

	Image {
		id: buttonImg
		// Workaround, can't get icon.source working for now...
		anchors.fill: parent
		source: "image://colorimage/" + image + "?" + (button.hovered ? colors.highlight : colors.buttonText)
	}

	MouseArea
	{
		id: mouseArea
		anchors.fill: parent
		onPressed:  mouse.accepted = false
		cursorShape: Qt.PointingHandCursor
	}
}
