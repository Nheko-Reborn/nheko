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

    property string roomid: ""
    property string roomName: ""
    property var onAccepted: undefined

    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowTitleHint
    width: 275
    height: 330
    minimumWidth: 250
    minimumHeight: 220

    EventExpiry {
        id: eventExpiry

        roomid: dialog.roomid
    }

    title: {
        if (roomid) {
            return qsTr("Event expiration for %1").arg(roomName);
        }
        else {
            return qsTr("Event expiration");
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
                    return qsTr("You can configure when your messages will be deleted in %1. This only happens when Nheko is open and has permissions to delete messages until Matrix servers support this feature natively. In general 0 means disable.").arg(roomName);
                }
                else {
                    return qsTr("You can configure when your messages will be deleted in all rooms unless configured otherwise. This only happens when Nheko is open and has permissions to delete messages until Matrix servers support this feature natively. In general 0 means disable.");
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
                text: qsTr("Expire events after X days")
                ToolTip.text: qsTr("Automatically redacts messages after X days, unless otherwise protected. Set to 0 to disable.")
                ToolTip.visible: hh1.hovered
                Layout.fillWidth: true

                HoverHandler {
                    id: hh1
                }
            }

            SpinBox {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                from: 0
                to: 1000
                stepSize: 1
                value: eventExpiry.expireEventsAfterDays
                onValueChanged: eventExpiry.expireEventsAfterDays = value
                editable: true
            }

            MatrixText {
                text: qsTr("Only keep latest X events")
                ToolTip.text: qsTr("Deletes your events in this room if there are more than X newer messages unless otherwise protected. Set to 0 to disable.")
                ToolTip.visible: hh2.hovered
                Layout.fillWidth: true

                HoverHandler {
                    id: hh2
                }
            }


            SpinBox {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                from: 0
                to: 1000000
                stepSize: 1
                value: eventExpiry.expireEventsAfterCount
                onValueChanged: eventExpiry.expireEventsAfterCount = value
                editable: true
            }

            MatrixText {
                text: qsTr("Always keep latest X events")
                ToolTip.text: qsTr("This prevents events to be deleted by the above 2 settings if they are the latest X messages from you in the room.")
                ToolTip.visible: hh3.hovered
                Layout.fillWidth: true

                HoverHandler {
                    id: hh3
                }
            }


            SpinBox {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                from: 0
                to: 1000000
                stepSize: 1
                value: eventExpiry.protectLatestEvents
                onValueChanged: eventExpiry.protectLatestEvents = value
                editable: true
            }

            MatrixText {
                text: qsTr("Include state events")
                ToolTip.text: qsTr("If this is turned on, old state events also get redacted. The latest state event of any type+key combination is excluded from redaction to not remove the room name and similar state by accident.")
                ToolTip.visible: hh4.hovered
                Layout.fillWidth: true

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

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        onAccepted: {
            eventExpiry.save();
            dialog.close();
        }
        onRejected: dialog.close();
    }

}

