// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import im.nheko 1.0

MatrixText {
    property string formatted: model.data.formattedBody
    property string copyText: selectedText ? getText(selectionStart, selectionEnd) : model.data.body

    text: "<style type=\"text/css\">a { color:" + colors.link + ";}\ncode { background-color: " + colors.alternateBase + ";}</style>" + formatted.replace("<pre>", "<pre style='white-space: pre-wrap; background-color: " + colors.alternateBase + "'>")
    width: parent ? parent.width : undefined
    height: isReply ? Math.round(Math.min(timelineRoot.height / 8, implicitHeight)) : undefined
    clip: isReply
    selectByMouse: !Settings.mobileMode && !isReply
    font.pointSize: (Settings.enlargeEmojiOnlyMessages && model.data.isOnlyEmoji > 0 && model.data.isOnlyEmoji < 4) ? Settings.fontSize * 3 : Settings.fontSize
}
