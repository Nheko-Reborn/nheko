// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko

Rectangle {
    property bool fitsMetadata: parent.width - redactedLayout.width > metadataWidth + 4
    property int metadataWidth

    color: timelineRoot.palette.alternateBase
    height: redactedLayout.implicitHeight + Nheko.paddingSmall
    implicitWidth: redactedLayout.implicitWidth + 2 * Nheko.paddingMedium
    radius: fontMetrics.lineSpacing / 2 + 2 * Nheko.paddingSmall
    width: Math.min(parent.width, implicitWidth + 1)

    RowLayout {
        id: redactedLayout
        anchors.centerIn: parent
        spacing: Nheko.paddingSmall
        width: parent.width - 2 * Nheko.paddingMedium

        Image {
            id: trashImg
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.preferredHeight: fontMetrics.font.pixelSize
            Layout.preferredWidth: fontMetrics.font.pixelSize
            source: "image://colorimage/:/icons/icons/ui/delete.svg?" + timelineRoot.palette.text
        }
        Label {
            id: redactedLabel

            property var redactedPair: room.formatRedactedEvent(eventId)

            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            Layout.fillWidth: true
            Layout.margins: 0
            Layout.preferredWidth: implicitWidth
            ToolTip.text: redactedPair["second"]
            ToolTip.visible: hh.hovered
            color: timelineRoot.palette.text
            text: redactedPair["first"]
            wrapMode: Label.WordWrap

            HoverHandler {
                id: hh
            }
        }
    }
}
