// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

Popup {
    id: snackbar

    property string currentMessage: ""
    property var messages: []

    function showNotification(msg) {
        messages.push(msg);
        currentMessage = messages[0];
        if (!visible) {
            open();
            dismissTimer.start();
        }
    }

    opacity: 0
    padding: Nheko.paddingLarge

    // Workaround palettes not inheriting for popups
    palette: timelineRoot.palette
    parent: Overlay.overlay
    x: (parent.width - width) / 2
    y: -100

    background: Rectangle {
        color: palette.dark
        opacity: 0.8
        radius: Nheko.paddingLarge
    }
    contentItem: Label {
        color: palette.light
        font.bold: true
        text: snackbar.currentMessage
        width: Math.max(snackbar.Overlay.overlay ? snackbar.Overlay.overlay.width / 2 : 0, 400)
    }
    enter: Transition {
        NumberAnimation {
            duration: 200
            easing.type: Easing.OutCubic
            from: 0.0
            property: "opacity"
            target: snackbar
            to: 1.0
        }
        NumberAnimation {
            duration: 1000
            easing.type: Easing.OutCubic
            from: -100
            properties: "y"
            target: snackbar
            to: 100
        }
    }
    exit: Transition {
        NumberAnimation {
            duration: 300
            easing.type: Easing.InCubic
            from: 1.0
            property: "opacity"
            target: snackbar
            to: 0.0
        }
        NumberAnimation {
            duration: 300
            easing.type: Easing.InCubic
            from: 100
            properties: "y"
            target: snackbar
            to: -100
        }
    }

    onAboutToHide: {
        messages.shift();
    }
    onClosed: {
        if (messages.length > 0) {
            currentMessage = messages[0];
            open();
            dismissTimer.restart();
        }
    }

    Timer {
        id: dismissTimer

        interval: 10000

        onTriggered: snackbar.close()
    }
}
