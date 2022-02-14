// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

Rectangle{

    height: redactedLayout.implicitHeight + Nheko.paddingSmall
    implicitWidth: redactedLayout.implicitWidth + 2 * Nheko.paddingMedium
    width: parent.width
    radius: fontMetrics.lineSpacing / 2 + 2 * Nheko.paddingSmall
    color: Nheko.colors.alternateBase

    RowLayout {
        id: redactedLayout
        anchors.centerIn: parent
        spacing: Nheko.paddingSmall

        Image {
            id: trashImg
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.preferredWidth: fontMetrics.font.pixelSize
            Layout.preferredHeight: fontMetrics.font.pixelSize
            source: "image://colorimage/:/icons/icons/ui/delete.svg?" + Nheko.colors.text
        }
        Label {
            id: redactedLabel
            Layout.margins: 0
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            Layout.preferredWidth: implicitWidth
            //Layout.fillWidth: true
            //Layout.maximumWidth: delegateWidth - 4 * Nheko.paddingSmall - trashImg.width - 2 * Nheko.paddingMedium
            property var redactedPair: room.formatRedactedEvent(eventId)
            text: redactedPair["first"]
            wrapMode: Label.WordWrap
            color: Nheko.colors.text

            ToolTip.text: redactedPair["second"]
            ToolTip.visible: hh.hovered
            HoverHandler {
                id: hh
            }
        }
    }
}
