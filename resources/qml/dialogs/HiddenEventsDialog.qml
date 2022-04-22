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

    property string roomid: ""
    property string roomName: ""
    property var onAccepted: undefined

    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowTitleHint
    width: 275
    height: 220
    minimumWidth: 250
    minimumHeight: 220

    HiddenEvents {
        id: hiddenEvents

        roomid: hiddenEventsDialog.roomid
    }

    title: {
        if (roomid) {
            return qsTr("Hidden events for %1").arg(roomName);
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
                if (roomid) {
                    return qsTr("These events will be <b>shown</b> in %1:").arg(roomName);
                }
                else {
                    return qsTr("These events will be <b>shown</b> in all rooms:");
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
                Layout.alignment: Qt.AlignRight
                checked: !hiddenEvents.hiddenEvents.includes(MtxEvent.Member)
                onToggled: hiddenEvents.toggle(MtxEvent.Member)
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
                Layout.alignment: Qt.AlignRight
                checked: !hiddenEvents.hiddenEvents.includes(MtxEvent.PowerLevels)
                onToggled: hiddenEvents.toggle(MtxEvent.PowerLevels)
            }

            MatrixText {
                text: qsTr("Stickers")
                Layout.fillWidth: true
            }

            ToggleButton {
                Layout.alignment: Qt.AlignRight
                checked: !hiddenEvents.hiddenEvents.includes(MtxEvent.Sticker)
                onToggled: hiddenEvents.toggle(MtxEvent.Sticker)
            }
        }
    }

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        onAccepted: {
            hiddenEvents.save();
            hiddenEventsDialog.close();
        }
        onRejected: hiddenEventsDialog.close();
    }

}
