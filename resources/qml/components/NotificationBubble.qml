// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

Rectangle {
    id: bubbleRoot

    required property color bubbleBackgroundColor
    required property color bubbleTextColor
    property alias font: notificationBubbleText.font
    required property bool hasLoudNotification
    property bool mayBeVisible: true
    required property int notificationCount

    ToolTip.delay: Nheko.tooltipDelay
    ToolTip.text: notificationCount
    ToolTip.visible: notificationBubbleHover.hovered && (notificationCount > 9999)
    baselineOffset: notificationBubbleText.baseline - bubbleRoot.top
    color: hasLoudNotification ? Nheko.theme.red : bubbleBackgroundColor
    implicitHeight: notificationBubbleText.height + Nheko.paddingMedium
    implicitWidth: Math.max(notificationBubbleText.width, height)
    radius: height / 2
    visible: mayBeVisible && notificationCount > 0

    Label {
        id: notificationBubbleText

        anchors.centerIn: bubbleRoot
        color: bubbleRoot.hasLoudNotification ? "white" : bubbleRoot.bubbleTextColor
        font.bold: true
        font.pixelSize: fontMetrics.font.pixelSize * 0.8
        horizontalAlignment: Text.AlignHCenter
        text: bubbleRoot.notificationCount > 9999 ? "9999+" : bubbleRoot.notificationCount
        verticalAlignment: Text.AlignVCenter
        width: Math.max(implicitWidth + Nheko.paddingMedium, bubbleRoot.height)

        HoverHandler {
            id: notificationBubbleHover

        }
    }
}
