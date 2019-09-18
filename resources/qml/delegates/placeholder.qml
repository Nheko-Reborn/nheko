import QtQuick 2.5
import QtQuick.Controls 2.5

Label {
	text: qsTr("unimplemented event: ") + eventData.type
	textFormat: Text.PlainText
	wrapMode: Text.Wrap
	width: parent.width
	color: inactiveColors.text
}
