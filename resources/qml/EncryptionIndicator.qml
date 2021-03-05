// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Controls 2.1
import im.nheko 1.0

Rectangle {
    id: indicator

    property bool encrypted: false

    function getEncryptionImage() {
        if (encrypted)
            return "image://colorimage/:/icons/icons/ui/lock.png?" + colors.buttonText;
        else
            return "image://colorimage/:/icons/icons/ui/unlock.png?#dd3d3d";
    }

    function getEncryptionTooltip() {
        if (encrypted)
            return qsTr("Encrypted");
        else
            return qsTr("This message is not encrypted!");
    }

    color: "transparent"
    width: 16
    height: 16
    ToolTip.visible: ma.hovered && indicator.visible
    ToolTip.text: getEncryptionTooltip()

    HoverHandler {
        id: ma
    }

    Image {
        id: stateImg

        anchors.fill: parent
        source: getEncryptionImage()
    }

}
