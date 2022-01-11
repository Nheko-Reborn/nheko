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

    property alias prompt: promptLabel.text
    property var onAccepted: undefined

    modality: Qt.NonModal
    flags: Qt.Dialog
    minimumWidth: 250
    minimumHeight: 220
    Component.onCompleted: Nheko.reparent(hiddenEventsDialog)
    title: qsTr("Hidden events settings for %1").arg(roomSettings.roomName)

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
                ToolTip.text: qsTr("Joins, leaves, invites, knocks and bans")
                ToolTip.visible: hh1.hovered
                Layout.fillWidth: true

                HoverHandler {
                    id: hh1
                }
            }

            ToggleButton {
                id: toggleRoomMember
                checked: roomSettings.eventHidden(0)
                Layout.alignment: Qt.AlignRight
            }

            MatrixText {
                text: qsTr("Power level changes")
                ToolTip.text: qsTr("Is sent when a moderator is added or removed or the permissions of a room are changed (happens a lot in some IRC rooms)")
                ToolTip.visible: hh2.hovered
                Layout.fillWidth: true

                HoverHandler {
                    id: hh2
                }
            }

            ToggleButton {
                id: toggleRoomPowerLevels
                checked: roomSettings.eventHidden(1)
                Layout.alignment: Qt.AlignRight
            }

            MatrixText {
                text: qsTr("Stickers")
                Layout.fillWidth: true
            }

            ToggleButton {
                id: toggleSticker
                Layout.alignment: Qt.AlignRight
                checked: roomSettings.eventHidden(2)
            }
        }
    }

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        onAccepted: {
            roomSettings.saveHiddenEventsSettings(toggleRoomMember.checked, toggleRoomPowerLevels.checked, toggleSticker.checked);

            hiddenEventsDialog.close();
        }
        onRejected: {
            hiddenEventsDialog.close();
        }
    }

}
