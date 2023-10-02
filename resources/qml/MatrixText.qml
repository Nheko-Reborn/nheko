// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

// TODO: using any Qt 6 API version will screw up the reply text color. We need to
//       figure out a more permanent fix than just importing the old version.
import QtQuick 2.15
import QtQuick.Controls
import im.nheko

TextEdit {
    id: r

    property alias cursorShape: cs.cursorShape

    //leftInset: 0
    //bottomInset: 0
    //rightInset: 0
    //topInset: 0
    //leftPadding: 0
    //bottomPadding: 0
    //rightPadding: 0
    //topPadding: 0
    //background: null

    ToolTip.text: hoveredLink
    ToolTip.visible: hoveredLink || false
    // this always has to be enabled, otherwise you can't click links anymore!
    //enabled: selectByMouse
    color: palette.text
    focus: false
    readOnly: true
    textFormat: TextEdit.RichText
    wrapMode: Text.Wrap

    // Setting a tooltip delay makes the hover text empty .-.
    //ToolTip.delay: Nheko.tooltipDelay
    Component.onCompleted: {
        TimelineManager.fixImageRendering(r.textDocument, r);
    }
    onLinkActivated: Nheko.openLink(link)

    //// propagate events up
    //onPressAndHold: (event) => event.accepted = false
    //onPressed: (event) => event.accepted = (event.button == Qt.LeftButton)

    NhekoCursorShape {
        id: cs

        anchors.fill: parent
        cursorShape: hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
