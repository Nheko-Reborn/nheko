import ".."

MatrixText {
	property string notice: model.data.formattedBody.replace("<pre>", "<pre style='white-space: pre-wrap'>")
	text: notice
	width: parent ? parent.width : undefined
	font.italic: true
	color: inactiveColors.text
}
