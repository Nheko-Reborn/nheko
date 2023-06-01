// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.5
import QtQuick.Controls 2.3
import im.nheko 1.0

TextEdit {
    id: r

    property alias cursorShape: cs.cursorShape

    ToolTip.text: hoveredLink
    ToolTip.visible: hoveredLink || false
    // this always has to be enabled, otherwise you can't click links anymore!
    //enabled: selectByMouse
    color: palette.text
    focus: false
    readOnly: true
    selectByMouse: !Settings.mobileMode
    textFormat: TextEdit.RichText
    wrapMode: Text.Wrap

    // Setting a tooltip delay makes the hover text empty .-.
    //ToolTip.delay: Nheko.tooltipDelay
    Component.onCompleted: {
        TimelineManager.fixImageRendering(r.textDocument, r);
    }
    onLinkActivated: Nheko.openLink(link)

    CursorShape {
        id: cs

        anchors.fill: parent
        cursorShape: hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
