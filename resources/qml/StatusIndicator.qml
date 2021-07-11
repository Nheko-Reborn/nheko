// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.5
import QtQuick.Controls 2.1
import im.nheko 1.0

ImageButton {
    id: indicator

    required property int status
    required property string eventId

    width: 16
    height: 16
    hoverEnabled: true
    changeColorOnHover: (status == MtxEvent.Read)
    cursor: (status == MtxEvent.Read) ? Qt.PointingHandCursor : Qt.ArrowCursor
    ToolTip.visible: hovered && status != MtxEvent.Empty
    ToolTip.text: {
        switch (status) {
        case MtxEvent.Failed:
            return qsTr("Failed");
        case MtxEvent.Sent:
            return qsTr("Sent");
        case MtxEvent.Received:
            return qsTr("Received");
        case MtxEvent.Read:
            return qsTr("Read");
        default:
            return "";
        }
    }
    onClicked: {
        if (status == MtxEvent.Read)
            room.readReceiptsAction(eventId);

    }
    image: {
        switch (status) {
        case MtxEvent.Failed:
            return ":/icons/icons/ui/remove-symbol.png";
        case MtxEvent.Sent:
            return ":/icons/icons/ui/clock.png";
        case MtxEvent.Received:
            return ":/icons/icons/ui/checkmark.png";
        case MtxEvent.Read:
            return ":/icons/icons/ui/double-tick-indicator.png";
        default:
            return "";
        }
    }
}
