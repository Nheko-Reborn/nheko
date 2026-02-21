// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import im.nheko

Image {
    required property var powerlevel
    required property Permissions permissions

    readonly property bool isV12Creator: permissions ? permissions.creatorLevel() == powerlevel : false
    readonly property bool isAdmin: permissions ? permissions.changeLevel(MtxEvent.PowerLevels) <= powerlevel : false
    readonly property bool isModerator: permissions ? permissions.redactLevel() <= powerlevel : false
    readonly property bool isDefault: permissions ? permissions.defaultLevel() <= powerlevel : false

    readonly property string sourceUrl: {
        if (isAdmin || isV12Creator)
             return "image://colorimage/:/icons/icons/ui/ribbon_star.svg?";
        else if (isModerator)
            return "image://colorimage/:/icons/icons/ui/ribbon.svg?";
        else
            return "image://colorimage/:/icons/icons/ui/person.svg?";
    }

    source: sourceUrl + (ma.hovered ? palette.highlight : palette.buttonText)
    ToolTip.visible: ma.hovered
    ToolTip.text: {
        let pl = powerlevel.toLocaleString(Qt.locale(), "f", 0);
        if (isV12Creator)
            return qsTr("Creator");
        else if (isAdmin)
            return qsTr("Administrator (%1)").arg(pl)
        else if (isModerator)
            return qsTr("Moderator: %1").arg(pl);
        else
            return qsTr("User: %1").arg(pl);
    }

    HoverHandler {
        id: ma
    }
}
