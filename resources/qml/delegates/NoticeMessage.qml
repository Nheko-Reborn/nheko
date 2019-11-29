import ".."

MatrixText {
	text: model.formattedBody
	width: parent ? parent.width : undefined
	font.italic: true
	color: inactiveColors.text
}
