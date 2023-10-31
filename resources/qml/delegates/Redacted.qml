// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

Control {
    id: msgRoot

    required property string eventId
    property bool fitsMetadata: false //parent.width - redactedLayout.width > metadataWidth + 4

    property int metadataWidth: 0
    required property Room room

    Layout.maximumWidth: redactedLayout.Layout.maximumWidth + padding * 2
    padding: Nheko.paddingSmall

    background: Rectangle {
        color: palette.alternateBase
        radius: fontMetrics.lineSpacing / 2 + 2 * Nheko.paddingSmall
    }
    contentItem: RowLayout {
        id: redactedLayout

        spacing: Nheko.paddingSmall

        Image {
            id: trashImg

            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.preferredHeight: fontMetrics.font.pixelSize
            Layout.preferredWidth: fontMetrics.font.pixelSize
            source: "image://colorimage/:/icons/icons/ui/delete.svg?" + palette.text
        }
        Label {
            id: redactedLabel

            property var redactedPair: msgRoot.room.formatRedactedEvent(msgRoot.eventId)

            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            Layout.fillWidth: true
            Layout.margins: 0
            Layout.maximumWidth: implicitWidth + 1
            ToolTip.text: redactedPair["second"]
            ToolTip.visible: hh.hovered
            text: redactedPair["first"]
            wrapMode: Label.WordWrap

            HoverHandler {
                id: hh

            }
        }
    }
}
