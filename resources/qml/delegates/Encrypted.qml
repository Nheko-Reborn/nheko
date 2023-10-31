// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

Control {
    id: r

    required property int encryptionError
    required property string eventId

    Layout.fillWidth: true
    Layout.maximumWidth: contents.Layout.maximumWidth + padding * 2
    implicitHeight: contents.implicitHeight + Nheko.paddingMedium * 2
    padding: Nheko.paddingMedium

    background: Rectangle {
        color: palette.alternateBase
        radius: fontMetrics.lineSpacing / 2 + 2 * Nheko.paddingMedium
        visible: !Settings.bubbles // the bubble in a bubble looks odd
    }
    contentItem: RowLayout {
        id: contents

        spacing: Nheko.paddingMedium

        Image {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 24
            Layout.preferredWidth: 24
            source: "image://colorimage/:/icons/icons/ui/shield-filled-cross.svg?" + Nheko.theme.error
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Nheko.paddingSmall

            Label {
                id: encryptedText

                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth + 1
                color: palette.text
                text: {
                    switch (r.encryptionError) {
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
                textFormat: Text.PlainText
                wrapMode: Label.WordWrap
            }
            Button {
                text: qsTr("Request key")
                visible: r.encryptionError == Olm.MissingSession || encryptionError == Olm.MissingSessionIndex

                onClicked: room.requestKeyForEvent(eventId)
            }
        }
    }
}
