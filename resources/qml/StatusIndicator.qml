import QtQuick 2.5
import QtQuick.Controls 2.1
import com.github.nheko 1.0

Rectangle {
	id: indicator
	property int state: 0
	color: "transparent"
	width: 16
	height: 16
	ToolTip.visible: ma.containsMouse && state != MtxEvent.Empty
	ToolTip.text: switch (state) {
		case MtxEvent.Failed: return qsTr("Failed")
		case MtxEvent.Sent: return qsTr("Sent")
		case MtxEvent.Received: return qsTr("Received")
		case MtxEvent.Read: return qsTr("Read")
		default: return ""
	}
	MouseArea{
		id: ma
		anchors.fill: parent
		hoverEnabled: true
	}

	Image {
		id: stateImg
		// Workaround, can't get icon.source working for now...
		anchors.fill: parent
		source: switch (indicator.state) {
			case MtxEvent.Failed: return "image://colorimage/:/icons/icons/ui/remove-symbol.png?" + colors.buttonText
			case MtxEvent.Sent: return "image://colorimage/:/icons/icons/ui/clock.png?" + colors.buttonText
			case MtxEvent.Received: return "image://colorimage/:/icons/icons/ui/checkmark.png?" + colors.buttonText
			case MtxEvent.Read: return "image://colorimage/:/icons/icons/ui/double-tick-indicator.png?" + colors.buttonText
			default: return ""
		}
	}
}

