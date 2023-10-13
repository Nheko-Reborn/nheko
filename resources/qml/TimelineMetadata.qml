// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./components"
import "./delegates"
import "./emoji"
import "./ui"
import "./dialogs"
import Qt.labs.platform 1.1 as Platform
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import im.nheko

RowLayout {
    id: metadata

    property int iconSize: Math.floor(fontMetrics.ascent * scaling)
    required property double scaling

    required property string eventId
    required property int status
    required property int trustlevel
    required property bool isEdited
    required property bool isEncrypted
    required property string threadId
    required property date timestamp
    required property Room room

    spacing: 2

    StatusIndicator {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        eventId: metadata.eventId
        height: parent.iconSize
        status: metadata.status
        width: parent.iconSize
    }
    Image {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        ToolTip.delay: Nheko.tooltipDelay
        ToolTip.text: qsTr("Edited")
        ToolTip.visible: editHovered.hovered
        height: parent.iconSize
        source: "image://colorimage/:/icons/icons/ui/edit.svg?" + ((metadata.eventId == metadata.room.edit) ? palette.highlight : palette.buttonText)
        sourceSize.height: parent.iconSize * Screen.devicePixelRatio
        sourceSize.width: parent.iconSize * Screen.devicePixelRatio
        visible: metadata.isEdited || metadata.eventId == metadata.room.edit
        width: parent.iconSize
        Layout.preferredWidth: parent.iconSize
        Layout.preferredHeight: parent.iconSize
        HoverHandler {
            id: editHovered

        }
    }
    ImageButton {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        ToolTip.delay: Nheko.tooltipDelay
        ToolTip.text: qsTr("Part of a thread")
        ToolTip.visible: hovered
        buttonTextColor: TimelineManager.userColor(metadata.threadId, palette.base)
        height: parent.iconSize
        image: ":/icons/icons/ui/thread.svg"
        visible: metadata.threadId
        width: parent.iconSize

        onClicked: metadata.room.thread = threadId
    }
    EncryptionIndicator {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        encrypted: metadata.isEncrypted
        height: parent.iconSize
        sourceSize.height: parent.iconSize * Screen.devicePixelRatio
        sourceSize.width: parent.iconSize * Screen.devicePixelRatio
        trust: metadata.trustlevel
        visible: metadata.room.isEncrypted
        width: parent.iconSize
        Layout.preferredWidth: parent.iconSize
        Layout.preferredHeight: parent.iconSize
    }
    Label {
        id: ts

        Layout.alignment: Qt.AlignRight | Qt.AlignTop
        Layout.preferredWidth: implicitWidth
        ToolTip.delay: Nheko.tooltipDelay
        ToolTip.text: Qt.formatDateTime(metadata.timestamp, Qt.DefaultLocaleLongDate)
        ToolTip.visible: ma.hovered
        color: palette.inactive.text
        font.pointSize: fontMetrics.font.pointSize * parent.scaling
        text: metadata.timestamp.toLocaleTimeString(Locale.ShortFormat)

        HoverHandler {
            id: ma

        }
    }
}
