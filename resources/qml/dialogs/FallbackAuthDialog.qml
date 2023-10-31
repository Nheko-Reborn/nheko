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
    flags: Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: msg.implicitHeight + footer.implicitHeight
    title: qsTr("Fallback authentication")
    width: Math.max(msg.implicitWidth, footer.implicitWidth)

    footer: DialogButtonBox {
        onAccepted: fallbackRoot.accept()
        onRejected: fallbackRoot.reject()

        Button {
            text: qsTr("Open Fallback in Browser")

            onClicked: fallback.openFallbackAuth()
        }
        Button {
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            text: qsTr("Cancel")
        }
        Button {
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            text: qsTr("Confirm")
        }
    }

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
}
