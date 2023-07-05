// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.5
import QtQuick.Controls 2.1
import im.nheko 1.0

ImageButton {
    id: indicator

    required property string eventId
    required property int status

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
    ToolTip.visible: hovered && status != MtxEvent.Empty
    changeColorOnHover: (status == MtxEvent.Read)
    cursor: (status == MtxEvent.Read) ? Qt.PointingHandCursor : Qt.ArrowCursor
    height: 16
    hoverEnabled: true
    image: {
        switch (status) {
        case MtxEvent.Failed:
            return ":/icons/icons/ui/dismiss.svg";
        case MtxEvent.Sent:
            return ":/icons/icons/ui/clock.svg";
        case MtxEvent.Received:
            return ":/icons/icons/ui/checkmark.svg";
        case MtxEvent.Read:
            return ":/icons/icons/ui/double-checkmark.svg";
        default:
            return "";
        }
    }
    width: 16

    onClicked: {
        if (status == MtxEvent.Read)
            room.showReadReceipts(eventId);
    }
}
