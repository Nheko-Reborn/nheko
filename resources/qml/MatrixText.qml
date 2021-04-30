// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.5
import QtQuick.Controls 2.3
import im.nheko 1.0

TextEdit {
    textFormat: TextEdit.RichText
    readOnly: true
    focus: false
    wrapMode: Text.Wrap
    selectByMouse: !Settings.mobileMode
    enabled: selectByMouse
    color: colors.text
    onLinkActivated: TimelineManager.openLink(link)
    ToolTip.visible: hoveredLink
    ToolTip.text: hoveredLink

    CursorShape {
        anchors.fill: parent
        cursorShape: hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

}
