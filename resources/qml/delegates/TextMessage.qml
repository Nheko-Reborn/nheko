import ".."

MatrixText {
	property string formatted: model.data.formattedBody
	text: "<style type=\"text/css\">a { color:"+colors.link+";}</style>" + formatted.replace("<pre>", "<pre style='white-space: pre-wrap'>")
	width: parent ? parent.width : undefined
}
