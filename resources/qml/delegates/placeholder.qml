import QtQuick 2.5
import QtQuick.Controls 2.1

Label {
	text: qsTr("unimplemented event: ") + model.type
	textFormat: Text.PlainText
	wrapMode: Text.Wrap
	width: parent.width
	color: inactiveColors.text
}
