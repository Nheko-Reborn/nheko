// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Controls 2.1
import QtQuick.Window 2.15
import im.nheko 1.0

Image {
    id: stateImg

    property bool encrypted: false
    property bool hovered: ma.hovered
    property string sourceUrl: {
        if (!encrypted)
            return "image://colorimage/" + unencryptedIcon + "?";
        switch (trust) {
        case Crypto.Verified:
            return "image://colorimage/:/icons/icons/ui/shield-filled-checkmark.svg?";
        case Crypto.TOFU:
            return "image://colorimage/:/icons/icons/ui/shield-filled.svg?";
        case Crypto.Unverified:
            return "image://colorimage/:/icons/icons/ui/shield-filled-exclamation-mark.svg?";
        default:
            return "image://colorimage/:/icons/icons/ui/shield-filled-cross.svg?";
        }
    }
    property int trust: Crypto.Unverified
    property color unencryptedColor: Nheko.theme.error
    property color unencryptedHoverColor: unencryptedColor
    property string unencryptedIcon: ":/icons/icons/ui/shield-filled-cross.svg"

    ToolTip.text: {
        if (!encrypted)
            return qsTr("This message is not encrypted!");
        switch (trust) {
        case Crypto.Verified:
            return qsTr("Encrypted by a verified device");
        case Crypto.TOFU:
            return qsTr("Encrypted by an unverified device, but you have trusted that user so far.");
        default:
            return qsTr("Encrypted by an unverified device or the key is from an untrusted source like the key backup.");
        }
    }
    ToolTip.visible: stateImg.hovered
    height: 16
    source: {
        if (encrypted) {
            switch (trust) {
            case Crypto.Verified:
                return sourceUrl + Nheko.theme.green;
            case Crypto.TOFU:
                return sourceUrl + palette.buttonText;
            default:
                return sourceUrl + Nheko.theme.error;
            }
        } else {
            return sourceUrl + (stateImg.hovered ? unencryptedHoverColor : unencryptedColor);
        }
    }
    sourceSize.height: height
    sourceSize.width: width
    width: 16

    HoverHandler {
        id: ma

    }
}
