// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick.Controls 2.3
import im.nheko 1.0

MatrixText {
    required property string body
    required property bool isOnlyEmoji
    required property bool isReply
    required property string formatted
    property string copyText: selectedText ? getText(selectionStart, selectionEnd) : body

    // table border-collapse doesn't seem to work
    text: "
    <style type=\"text/css\">
    a { color:" + Nheko.colors.link + ";}
    code { background-color: " + Nheko.colors.alternateBase + ";}
    table {
        border-width: 1px;
        border-collapse: collapse;
        border-style: solid;
    }
    table th,
    table td {
        bgcolor: " + Nheko.colors.alternateBase + ";
        border-collapse: collapse;
        border: 1px solid " + Nheko.colors.text + ";
    }
    blockquote { margin-left: 1em; }
    </style>
    " + formatted.replace("<pre>", "<pre style='white-space: pre-wrap; background-color: " + Nheko.colors.alternateBase + "'>").replace("<del>", "<s>").replace("</del>", "</s>").replace("<strike>", "<s>").replace("</strike>", "</s>")
    width: parent.width
    height: isReply ? Math.round(Math.min(timelineView.height / 8, implicitHeight)) : implicitHeight
    clip: isReply
    selectByMouse: !Settings.mobileMode && !isReply
    font.pointSize: (Settings.enlargeEmojiOnlyMessages && isOnlyEmoji > 0 && isOnlyEmoji < 4) ? Settings.fontSize * 3 : Settings.fontSize

    CursorShape {
        enabled: isReply
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

}
