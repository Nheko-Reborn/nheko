// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

ApplicationWindow {
    id: dialog

    property var onAccepted: undefined
    property string roomName: ""
    property string roomid: ""

    flags: Qt.Dialog | Qt.WindowTitleHint
    height: 330
    minimumHeight: 220
    minimumWidth: 250
    modality: Qt.NonModal
    title: {
        if (roomid) {
            return qsTr("Event expiration for %1").arg(roomName);
        } else {
            return qsTr("Event expiration");
        }
    }
    width: 275

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onAccepted: {
            eventExpiry.save();
            dialog.close();
        }
        onRejected: dialog.close()
    }

    EventExpiry {
        id: eventExpiry

        roomid: dialog.roomid
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
                    return qsTr("You can configure when your messages will be deleted in %1. This only happens when Nheko is open and has permissions to delete messages until Matrix servers support this feature natively. In general 0 means disable.").arg(roomName);
                } else {
                    return qsTr("You can configure when your messages will be deleted in all rooms unless configured otherwise. This only happens when Nheko is open and has permissions to delete messages until Matrix servers support this feature natively. In general 0 means disable.");
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
                ToolTip.text: qsTr("Automatically redacts messages after X days, unless otherwise protected. Set to 0 to disable.")
                ToolTip.visible: hh1.hovered
                text: qsTr("Expire events after X days")

                HoverHandler {
                    id: hh1

                }
            }
            SpinBox {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                editable: true
                from: 0
                stepSize: 1
                to: 1000
                value: eventExpiry.expireEventsAfterDays

                onValueChanged: eventExpiry.expireEventsAfterDays = value
            }
            MatrixText {
                Layout.fillWidth: true
                ToolTip.text: qsTr("Deletes your events in this room if there are more than X newer messages unless otherwise protected. Set to 0 to disable.")
                ToolTip.visible: hh2.hovered
                text: qsTr("Only keep latest X events")

                HoverHandler {
                    id: hh2

                }
            }
            SpinBox {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                editable: true
                from: 0
                stepSize: 1
                to: 1000000
                value: eventExpiry.expireEventsAfterCount

                onValueChanged: eventExpiry.expireEventsAfterCount = value
            }
            MatrixText {
                Layout.fillWidth: true
                ToolTip.text: qsTr("This prevents events to be deleted by the above 2 settings if they are the latest X messages from you in the room.")
                ToolTip.visible: hh3.hovered
                text: qsTr("Always keep latest X events")

                HoverHandler {
                    id: hh3

                }
            }
            SpinBox {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                editable: true
                from: 0
                stepSize: 1
                to: 1000000
                value: eventExpiry.protectLatestEvents

                onValueChanged: eventExpiry.protectLatestEvents = value
            }
            MatrixText {
                Layout.fillWidth: true
                ToolTip.text: qsTr("If this is turned on, old state events also get redacted. The latest state event of any type+key combination is excluded from redaction to not remove the room name and similar state by accident.")
                ToolTip.visible: hh4.hovered
                text: qsTr("Include state events")

                HoverHandler {
                    id: hh4

                }
            }
            ToggleButton {
                Layout.alignment: Qt.AlignRight
                checked: eventExpiry.expireStateEvents

                onToggled: eventExpiry.expireStateEvents = checked
            }
        }
    }
}
