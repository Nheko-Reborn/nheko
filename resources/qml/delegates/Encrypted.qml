// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

Rectangle {
    id: r

    required property int encryptionError
    required property string eventId

    radius: fontMetrics.lineSpacing / 2 + Nheko.paddingMedium
    width: parent.width? parent.width : 0
    implicitWidth: encryptedText.implicitWidth+24+Nheko.paddingMedium*3 // Column doesn't provide a useful implicitWidth, should be replaced by ColumnLayout
    height: contents.implicitHeight + Nheko.paddingMedium * 2
    color: Nheko.colors.alternateBase

    RowLayout {
        id: contents

        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Image {
            source: "image://colorimage/:/icons/icons/ui/shield-filled-cross.svg?" + Nheko.theme.error
            Layout.alignment: Qt.AlignVCenter
            width: 24
            height: width
        }

        Column {
            spacing: Nheko.paddingSmall
            Layout.fillWidth: true

            MatrixText {
                id: encryptedText
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
                color: Nheko.colors.text
                width: parent.width
            }

            Button {
                palette: Nheko.colors
                visible: encryptionError == Olm.MissingSession || encryptionError == Olm.MissingSessionIndex
                text: qsTr("Request key")
                onClicked: room.requestKeyForEvent(eventId)
            }

        }

    }

}
