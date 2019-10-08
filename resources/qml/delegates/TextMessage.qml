import QtQuick 2.5

TextEdit {
	text: model.formattedBody
	textFormat: TextEdit.RichText
	readOnly: true
	wrapMode: Text.Wrap
	width: parent.width
	selectByMouse: true
	color: colors.text
}
