import QtQuick 2.5

TextEdit {
	text: model.formattedBody
	textFormat: TextEdit.RichText
	readOnly: true
	wrapMode: Text.Wrap
	width: parent ? parent.width : undefined
	selectByMouse: true
	font.italic: true
	color: inactiveColors.text
}
