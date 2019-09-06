import QtQuick 2.5

TextEdit {
	text: eventData.formattedBody
	textFormat: TextEdit.RichText
	readOnly: true
	wrapMode: Text.Wrap
	width: parent.width
	selectByMouse: true
	font.italic: true
	color: inactiveColors.text
}
