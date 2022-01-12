// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko 1.0

ApplicationWindow {
    id: hiddenEventsDialog

    property var isRoomSetting: false
    property var onAccepted: undefined

    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowTitleHint
    minimumWidth: 250
    minimumHeight: 220
    Component.onCompleted: Nheko.reparent(hiddenEventsDialog)
    title: {
        if (isRoomSetting) {
            return qsTr("Hidden events for %1").arg(roomSettings.roomName);
        }
        else {
            return qsTr("Hidden events");
        }
    }

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: dbb.rejected()
    }

    ColumnLayout {
        spacing: Nheko.paddingMedium
        anchors.margins: Nheko.paddingMedium
        anchors.fill: parent

        MatrixText {
            id: promptLabel
            text: {
                if (isRoomSetting) {
                    return qsTr("These events will be be <b>shown</b> in %1:").arg(roomSettings.roomName);
                }
                else {
                    return qsTr("These events will be be <b>shown</b>:");
                }
            }
            font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.2)
            Layout.fillWidth: true
            Layout.fillHeight: false
        }

        GridLayout {
            columns: 2
            rowSpacing: Nheko.paddingMedium
            Layout.fillWidth: true
            Layout.fillHeight: true

            MatrixText {
                text: qsTr("User events")
                ToolTip.text: qsTr("Joins, leaves, avatar and name changes, bans, …")
                ToolTip.visible: hh1.hovered
                Layout.fillWidth: true

                HoverHandler {
                    id: hh1
                }
            }

            ToggleButton {
                id: toggleRoomMember
                checked: !roomSettings.eventHidden("m.room.member")
                Layout.alignment: Qt.AlignRight
            }

            MatrixText {
                text: qsTr("Power level changes")
                ToolTip.text: qsTr("Sent when a moderator is added/removed or the permissions of a room are changed.")
                ToolTip.visible: hh2.hovered
                Layout.fillWidth: true

                HoverHandler {
                    id: hh2
                }
            }

            ToggleButton {
                id: toggleRoomPowerLevels
                checked: !roomSettings.eventHidden("m.room.power_levels")
                Layout.alignment: Qt.AlignRight
            }

            MatrixText {
                text: qsTr("Stickers")
                Layout.fillWidth: true
            }

            ToggleButton {
                id: toggleSticker
                Layout.alignment: Qt.AlignRight
                checked: !roomSettings.eventHidden("m.sticker")
            }
        }
    }

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        onAccepted: {
            let events = new Array;
            if (!toggleRoomMember.checked) {
                events.push("m.room.member");
            }
            if (!toggleRoomPowerLevels.checked) {
                events.push("m.room.power_levels");
            }
            if (!toggleSticker.checked) {
                events.push("m.sticker");
            }
            roomSettings.saveHiddenEventsSettings(events);

            hiddenEventsDialog.close();
        }
        onRejected: {
            hiddenEventsDialog.close();
        }
    }

}
