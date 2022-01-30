// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

Popup {
    id: snackbar

    property var messages: []
    property string currentMessage: ""

    function showNotification(msg) {
        messages.push(msg);
        currentMessage = messages[0];
        if (!visible) {
            open();
            dismissTimer.start();
        }
    }

    Timer {
        id: dismissTimer
        interval: 10000
        onTriggered: snackbar.close()
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

    parent: Overlay.overlay
    opacity: 0
    y: -100
    x: (parent.width - width)/2
    padding: Nheko.paddingLarge

    contentItem: Label {
        color: Nheko.colors.light
        width: Math.max(Overlay.overlay? Overlay.overlay.width/2 : 0, 400)
        text: snackbar.currentMessage
        font.bold: true
    }

    background: Rectangle {
        radius: Nheko.paddingLarge
        color: Nheko.colors.dark
        opacity: 0.8
    }

    enter: Transition {
        NumberAnimation {
            target: snackbar
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 200
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: snackbar
            properties: "y"
            from: -100
            to: 100
            duration: 1000
            easing.type: Easing.OutCubic
        }
    }
    exit: Transition {
        NumberAnimation {
            target: snackbar
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 300
            easing.type: Easing.InCubic
        }
        NumberAnimation {
            target: snackbar
            properties: "y"
            to: -100
            from: 100
            duration: 300
            easing.type: Easing.InCubic
        }
    }
}


