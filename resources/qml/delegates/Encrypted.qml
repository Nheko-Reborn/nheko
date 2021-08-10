// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.2
import im.nheko 1.0

ColumnLayout {
    id: r

    required property int encryptionError
    required property string eventId

    width: parent ? parent.width : undefined

    MatrixText {
        text: {
            switch (encryptionError) {
            case Olm.MissingSession:
                return qsTr("There is no key to unlock this message. We requested the key automatically, but you can try requesting it again if you are impatient.");
            case Olm.MissingSessionIndex:
                return qsTr("This message couldn't be decrypted, because we only have a key for newer messages. You can try requesting access to this message.");
            case Olm.DbError:
                return qsTr("There was an internal error reading the decryption key from the database.");
            case Olm.DecryptionFailed:
                return qsTr("There was an error decrypting this message.");
            case Olm.ParsingFailed:
                return qsTr("The message couldn't be parsed.");
            case Olm.ReplayAttack:
                return qsTr("The encryption key was reused! Someone is possibly trying to insert false messages into this chat!");
            default:
                return qsTr("Unknown decryption error");
            }
        }
        color: Nheko.colors.buttonText
        width: r ? r.width : undefined
    }

    Button {
        palette: Nheko.colors
        visible: encryptionError == Olm.MissingSession || encryptionError == Olm.MissingSessionIndex
        text: qsTr("Request key")
        onClicked: room.requestKeyForEvent(eventId)
    }

}
