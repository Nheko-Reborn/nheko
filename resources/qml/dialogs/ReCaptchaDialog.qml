// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import im.nheko

ApplicationWindow {
    id: recaptchaRoot

    required property ReCaptcha recaptcha

    function accept() {
        recaptcha.confirm();
        recaptchaRoot.close();
    }

    function reject() {
        recaptcha.cancel();
        recaptchaRoot.close();
    }

    color: palette.window
    title: recaptcha.context
    flags: Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: msg.implicitHeight + footer.implicitHeight
    width: Math.max(msg.implicitWidth, footer.implicitWidth)

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: recaptchaRoot.reject()
    }

    Label {
        id: msg

        anchors.fill: parent
        padding: 8
        text: qsTr("Solve the reCAPTCHA and press the confirm button")
    }

    footer: DialogButtonBox {
        onAccepted: recaptchaRoot.accept()
        onRejected: recaptchaRoot.reject()

        Button {
            text: qsTr("Open reCAPTCHA")
            onClicked: recaptcha.openReCaptcha()
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
