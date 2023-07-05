// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.5
import im.nheko 1.0

Label {
    id: root

    property alias elideWidth: metrics.elideWidth
    property alias fullText: metrics.text
    property int fullTextWidth: Math.ceil(metrics.advanceWidth)

    color: palette.text
    elide: Text.ElideRight
    maximumLineCount: 1
    text: (textFormat == Text.PlainText) ? metrics.elidedText : TimelineManager.escapeEmoji(metrics.elidedText)
    textFormat: Text.PlainText

    TextMetrics {
        id: metrics

        elide: Text.ElideRight
        font.pointSize: root.font.pointSize
    }
}
