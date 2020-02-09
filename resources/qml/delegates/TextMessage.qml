import ".."

MatrixText {
	property string formatted: model.data.formattedBody
	text: formatted.replace("<pre>", "<pre style='white-space: pre-wrap'>")
	width: parent ? parent.width : undefined
}
