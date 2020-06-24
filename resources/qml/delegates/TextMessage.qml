import ".."

import im.nheko 1.0

MatrixText {
	property string formatted: model.data.formattedBody
	text: "<style type=\"text/css\">a { color:"+colors.link+";}</style>" + formatted.replace("<pre>", "<pre style='white-space: pre-wrap'>")
	width: parent ? parent.width : undefined
	height: isReply ? Math.min(chat.height / 8, implicitHeight) : undefined
	clip: true
	font.pointSize: (Settings.enlargeEmojiOnlyMessages && model.data.isOnlyEmoji > 0 && model.data.isOnlyEmoji < 4) ? Settings.fontSize * 3 : Settings.fontSize
}
