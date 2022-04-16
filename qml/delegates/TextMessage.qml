// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import QtQuick
import QtQuick.Controls
import im.nheko

MatrixText {
    required property string body
    property string copyText: selectedText ? getText(selectionStart, selectionEnd) : body
    property bool fitsMetadata: positionAt(width, height - 4) == positionAt(width - metadataWidth - 10, height - 4)
    required property string formatted
    required property bool isOnlyEmoji
    required property bool isReply
    property int metadataWidth

    clip: isReply
    enabled: !Settings.mobileMode
    font.pointSize: (Settings.enlargeEmojiOnlyMessages && isOnlyEmoji > 0 && isOnlyEmoji < 4) ? Settings.fontSize * 3 : Settings.fontSize
    height: isReply ? Math.round(Math.min(timelineView.height / 8, implicitHeight)) : implicitHeight
    selectByMouse: !Settings.mobileMode && !isReply

    // table border-collapse doesn't seem to work
    text: "
    <style type=\"text/css\">
    a { color:" + timelineRoot.palette.link + ";}
    code { background-color: " + timelineRoot.palette.alternateBase + ";}
    table {
        border-width: 1px;
        border-collapse: collapse;
        border-style: solid;
    }
    table th,
    table td {
        bgcolor: " + timelineRoot.palette.alternateBase + ";
        border-collapse: collapse;
        border: 1px solid " + timelineRoot.palette.text + ";
    }
    blockquote { margin-left: 1em; }
    </style>
    " + formatted.replace(/<pre>/g, "<pre style='white-space: pre-wrap; background-color: " + timelineRoot.palette.alternateBase + "'>").replace(/<del>/g, "<s>").replace(/<\/del>/g, "</s>").replace(/<strike>/g, "<s>").replace(/<\/strike>/g, "</s>")
    width: parent?.width

    NhekoCursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        enabled: isReply
    }
}
