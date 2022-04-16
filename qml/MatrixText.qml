// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.5
import QtQuick.Controls 2.3
import im.nheko

TextEdit {
    id: r

    property alias cursorShape: cs.cursorShape

    ToolTip.text: hoveredLink
    ToolTip.visible: hoveredLink || false
    // this always has to be enabled, otherwise you can't click links anymore!
    //enabled: selectByMouse
    color: timelineRoot.palette.text
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

    NhekoCursorShape {
        id: cs
        anchors.fill: parent
        cursorShape: hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
