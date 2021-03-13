// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Controls 2.1
import im.nheko 1.0

Image {
    id: stateImg

    property bool encrypted: false

    width: 16
    height: 16
    source: {
        if (encrypted)
            return "image://colorimage/:/icons/icons/ui/lock.png?" + colors.buttonText;
        else
            return "image://colorimage/:/icons/icons/ui/unlock.png?#dd3d3d";
    }
    ToolTip.visible: ma.hovered
    ToolTip.text: encrypted ? qsTr("Encrypted") : qsTr("This message is not encrypted!")

    HoverHandler {
        id: ma
    }

}
