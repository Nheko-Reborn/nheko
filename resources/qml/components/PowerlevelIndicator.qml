// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import im.nheko

Image {
    readonly property bool isAdmin: permissions ? permissions.changeLevel(MtxEvent.PowerLevels) <= powerlevel : false
    readonly property bool isDefault: permissions ? permissions.defaultLevel() <= powerlevel : false
    readonly property bool isModerator: permissions ? permissions.redactLevel() <= powerlevel : false
    required property var permissions
    required property int powerlevel
    readonly property string sourceUrl: {
        if (isAdmin)
            return "image://colorimage/:/icons/icons/ui/ribbon_star.svg?";
        else if (isModerator)
            return "image://colorimage/:/icons/icons/ui/ribbon.svg?";
        else
            return "image://colorimage/:/icons/icons/ui/person.svg?";
    }

    ToolTip.text: {
        if (isAdmin)
            return qsTr("Administrator: %1").arg(powerlevel);
        else if (isModerator)
            return qsTr("Moderator: %1").arg(powerlevel);
        else
            return qsTr("User: %1").arg(powerlevel);
    }
    ToolTip.visible: ma.hovered
    source: sourceUrl + (ma.hovered ? palette.highlight : palette.buttonText)

    HoverHandler {
        id: ma

    }
}
