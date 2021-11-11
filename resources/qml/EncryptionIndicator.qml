// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Controls 2.1
import im.nheko 1.0

Image {
    id: stateImg

    property bool encrypted: false
    property int trust: Crypto.Unverified

    width: 16
    height: 16
    source: {
        if (encrypted) {
            switch (trust) {
            case Crypto.Verified:
                return "image://colorimage/:/icons/icons/ui/lock.png?green";
            case Crypto.TOFU:
                return "image://colorimage/:/icons/icons/ui/lock.png?" + Nheko.colors.buttonText;
            default:
                return "image://colorimage/:/icons/icons/ui/lock.png?#d6c020";
            }
        } else {
            return "image://colorimage/:/icons/icons/ui/unlock.png?#d6c020";
        }
    }
    ToolTip.visible: ma.hovered
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

    HoverHandler {
        id: ma
    }

}
