// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import ".."
import im.nheko

MatrixText {
    required property string body
    required property bool isOnlyEmoji
    property bool isReply: EventDelegateChooser.isReply
    required property bool keepFullText
    required property string formatted

    property string copyText: selectedText ? getText(selectionStart, selectionEnd) : body
    property int metadataWidth: 100
    property bool fitsMetadata: false //positionAt(width,height-4) == positionAt(width-metadataWidth-10, height-4)
    property bool showSpoilers: false

    // table border-collapse doesn't seem to work
    text: `
    <style type="text/css">
    code { background-color: ` + palette.alternateBase + `; white-space: pre-wrap; }
    pre { background-color: ` + palette.alternateBase + `; white-space: pre-wrap; }
    table {
        border-width: 1px;
        border-collapse: collapse;
        border-style: solid;
        border-color: ` + palette.text + `;
        background-color: ` + palette.alternateBase + `;
    }
    table th,
    table td {
        padding: ` + Math.ceil(fontMetrics.lineSpacing/2) + `px;
    }
    blockquote { margin-left: 1em; }
    span[data-mx-spoiler] {` + (!showSpoilers ? `
        color: transparent;
        background-color: ` + palette.text + `;` : `
        background-color: ` + palette.alternateBase + ';') + `
    }
    </style>
    ` + formatted.replace(/<del>/g, "<s>").replace(/<\/del>/g, "</s>").replace(/<strike>/g, "<s>").replace(/<\/strike>/g, "</s>")

    enabled: !isReply
    font.pointSize: (Settings.enlargeEmojiOnlyMessages && isOnlyEmoji > 0 && isOnlyEmoji < 4) ? Settings.fontSize * 3 : Settings.fontSize

    NhekoCursorShape {
        enabled: isReply
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        onTapped: showSpoilers = !showSpoilers
    }
}
