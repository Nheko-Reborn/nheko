import ".."

MatrixText {
	property string formatted: model.data.formattedBody
	text: "<style type=\"text/css\">a { color:"+colors.link+";}</style>" + formatted.replace("<pre>", "<pre style='white-space: pre-wrap'>")
	width: parent ? parent.width : undefined
	height: isReply ? Math.round(Math.min(timelineRoot.height / 8, implicitHeight)) : undefined
	clip: true
	font.pointSize: (settings.enlargeEmojiOnlyMessages && model.data.isOnlyEmoji > 0 && model.data.isOnlyEmoji < 4) ? settings.fontSize * 3 : settings.fontSize
}
