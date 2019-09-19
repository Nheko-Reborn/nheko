import QtQuick 2.5
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.0
import com.github.nheko 1.0

Rectangle {
	id: indicator
	color: "transparent"
	width: 16
	height: 16
	ToolTip.visible: ma.containsMouse && indicator.visible
	ToolTip.text: qsTr("Encrypted")
	MouseArea{
		id: ma
		anchors.fill: parent
		hoverEnabled: true
	}

	Image {
		id: stateImg
		anchors.fill: parent
		source: "qrc:/icons/icons/ui/lock.png"
	}
	ColorOverlay {
		anchors.fill: stateImg
		source: stateImg
		color: colors.buttonText
	}
}

