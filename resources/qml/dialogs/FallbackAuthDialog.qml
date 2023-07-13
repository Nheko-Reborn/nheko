// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import im.nheko

ApplicationWindow {
    id: fallbackRoot

    required property FallbackAuth fallback

    function accept() {
        fallback.confirm();
        fallbackRoot.close();
    }

    function reject() {
        fallback.cancel();
        fallbackRoot.close();
    }

    color: palette.window
    title: qsTr("Fallback authentication")
    flags: Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: msg.implicitHeight + footer.implicitHeight
    width: Math.max(msg.implicitWidth, footer.implicitWidth)

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: fallbackRoot.reject()
    }

    Label {
        id: msg

        anchors.fill: parent
        padding: 8
        text: qsTr("Open the fallback, follow the steps, and confirm after completing them.")
    }

    footer: DialogButtonBox {
        onAccepted: fallbackRoot.accept()
        onRejected: fallbackRoot.reject()

        Button {
            text: qsTr("Open Fallback in Browser")
            onClicked: fallback.openFallbackAuth()
        }

        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }

        Button {
            text: qsTr("Confirm")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

}
