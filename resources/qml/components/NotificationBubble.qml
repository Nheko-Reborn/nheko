// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

Rectangle {
    id: bubbleRoot

    required property int notificationCount
    required property bool hasLoudNotification
    required property color bubbleBackgroundColor
    required property color bubbleTextColor
    property bool mayBeVisible: true
    property alias font: notificationBubbleText.font

    visible: mayBeVisible && notificationCount > 0
    implicitHeight: notificationBubbleText.height + Nheko.paddingMedium
    implicitWidth: Math.max(notificationBubbleText.width, height)
    radius: height / 2
    color: hasLoudNotification ? Nheko.theme.red : bubbleBackgroundColor
    ToolTip.text: notificationCount
    ToolTip.delay: Nheko.tooltipDelay
    ToolTip.visible: notificationBubbleHover.hovered && (notificationCount > 9999)

    Label {
        id: notificationBubbleText

        anchors.centerIn: bubbleRoot
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        width: Math.max(implicitWidth + Nheko.paddingMedium, bubbleRoot.height)
        font.bold: true
        font.pixelSize: fontMetrics.font.pixelSize * 0.8
        color: bubbleRoot.hasLoudNotification ? "white" : bubbleRoot.bubbleTextColor
        text: bubbleRoot.notificationCount > 9999 ? "9999+" : bubbleRoot.notificationCount

        HoverHandler {
            id: notificationBubbleHover
        }

    }

}
