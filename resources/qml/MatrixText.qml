// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.5
import QtQuick.Controls 2.3
import im.nheko 1.0

TextEdit {
    id: r

    property alias cursorShape: cs.cursorShape

    textFormat: TextEdit.RichText
    readOnly: true
    focus: false
    wrapMode: Text.Wrap
    selectByMouse: !Settings.mobileMode
    // this always has to be enabled, otherwise you can't click links anymore!
    //enabled: selectByMouse
    color: Nheko.colors.text
    onLinkActivated: Nheko.openLink(link)
    ToolTip.visible: hoveredLink || false
    ToolTip.text: hoveredLink
    // Setting a tooltip delay makes the hover text empty .-.
    //ToolTip.delay: Nheko.tooltipDelay
    Component.onCompleted: {
        TimelineManager.fixImageRendering(r.textDocument, r);
    }

    CursorShape {
        id: cs

        anchors.fill: parent
        cursorShape: hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

}
