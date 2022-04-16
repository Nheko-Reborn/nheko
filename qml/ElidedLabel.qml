// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.9
import QtQuick.Controls 2.5
import im.nheko

Label {
    id: root

    property alias elideWidth: metrics.elideWidth
    property alias fullText: metrics.text
    property int fullTextWidth: Math.ceil(metrics2.advanceWidth + 4)

    color: timelineRoot.palette.text
    elide: Text.ElideRight
    maximumLineCount: 1
    text: (textFormat == Text.PlainText) ? metrics.elidedText : TimelineManager.escapeEmoji(TimelineManager.htmlEscape(metrics.elidedText))
    textFormat: Text.PlainText

    TextMetrics {
        id: metrics
        elide: Text.ElideRight
        font: root.font
    }
    TextMetrics {
        id: metrics2
        //elide: Text.ElideRight
        font: root.font
        text: metrics.text
    }
}
