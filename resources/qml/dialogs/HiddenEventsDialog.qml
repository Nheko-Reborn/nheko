// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko 1.0

ApplicationWindow {
    id: hiddenEventsDialog

    property var onAccepted: undefined
    property string roomName: ""
    property string roomid: ""

    flags: Qt.Dialog | Qt.WindowTitleHint
    height: 220
    minimumHeight: 220
    minimumWidth: 250
    modality: Qt.NonModal
    title: {
        if (roomid) {
            return qsTr("Hidden events for %1").arg(roomName);
        } else {
            return qsTr("Hidden events");
        }
    }
    width: 275

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onAccepted: {
            hiddenEvents.save();
            hiddenEventsDialog.close();
        }
        onRejected: hiddenEventsDialog.close()
    }

    HiddenEvents {
        id: hiddenEvents

        roomid: hiddenEventsDialog.roomid
    }
    Shortcut {
        sequence: StandardKey.Cancel

        onActivated: dbb.rejected()
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        MatrixText {
            id: promptLabel

            Layout.fillHeight: false
            Layout.fillWidth: true
            font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.2)
            text: {
                if (roomid) {
                    return qsTr("These events will be <b>shown</b> in %1:").arg(roomName);
                } else {
                    return qsTr("These events will be <b>shown</b> in all rooms:");
                }
            }
        }
        GridLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            columns: 2
            rowSpacing: Nheko.paddingMedium

            MatrixText {
                Layout.fillWidth: true
                ToolTip.text: qsTr("Joins, leaves, avatar and name changes, bans, …")
                ToolTip.visible: hh1.hovered
                text: qsTr("User events")

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
                Layout.fillWidth: true
                ToolTip.text: qsTr("Sent when a moderator is added/removed or the permissions of a room are changed.")
                ToolTip.visible: hh2.hovered
                text: qsTr("Power level changes")

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
                Layout.fillWidth: true
                text: qsTr("Stickers")
            }
            ToggleButton {
                Layout.alignment: Qt.AlignRight
                checked: !hiddenEvents.hiddenEvents.includes(MtxEvent.Sticker)

                onToggled: hiddenEvents.toggle(MtxEvent.Sticker)
            }
        }
    }
}
