// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick.Controls 2.3
import im.nheko 1.0

MatrixText {
    required property string body
    required property bool isOnlyEmoji
    required property bool isReply
    required property bool keepFullText
    required property string formatted
    property string copyText: selectedText ? getText(selectionStart, selectionEnd) : body
    property int metadataWidth
    property bool fitsMetadata: false //positionAt(width,height-4) == positionAt(width-metadataWidth-10, height-4)

    // table border-collapse doesn't seem to work
    text: "
    <style type=\"text/css\">
    a { color:" + palette.link + ";}
    code { background-color: " + palette.alternateBase + "; white-space: pre-wrap; }
    pre { background-color: " + palette.alternateBase + "; white-space: pre-wrap; }
    table {
        border-width: 1px;
        border-collapse: collapse;
        border-style: solid;
        border-color: " + palette.text + ";
        background-color: " + palette.alternateBase + ";
    }
    table th,
    table td {
        padding: " + Math.ceil(fontMetrics.lineSpacing/2) + "px;
    }
    blockquote { margin-left: 1em; }
    " + (!Settings.mobileMode ? "span[data-mx-spoiler] {
        color: transparent;
        background-color: " + palette.text + ";
    }" : "") +  // TODO(Nico): Figure out how to support mobile
    "</style>
    " + formatted.replace(/<del>/g, "<s>").replace(/<\/del>/g, "</s>").replace(/<strike>/g, "<s>").replace(/<\/strike>/g, "</s>")
    width: parent?.width ?? 0
    height: !keepFullText ? Math.round(Math.min(timelineView.height / 8, implicitHeight)) : implicitHeight
    clip: !keepFullText
    selectByMouse: !Settings.mobileMode && !isReply
    enabled: !Settings.mobileMode
    font.pointSize: (Settings.enlargeEmojiOnlyMessages && isOnlyEmoji > 0 && isOnlyEmoji < 4) ? Settings.fontSize * 3 : Settings.fontSize

    CursorShape {
        enabled: isReply
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

}
