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

    padding: Nheko.paddingMedium
    implicitHeight: contents.implicitHeight + Nheko.paddingMedium * 2
    Layout.maximumWidth: contents.Layout.maximumWidth + padding * 2
    Layout.fillWidth: true

    contentItem: RowLayout {
        id: contents

        spacing: Nheko.paddingMedium

        Image {
            source: "image://colorimage/:/icons/icons/ui/shield-filled-cross.svg?" + Nheko.theme.error
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
        }

        ColumnLayout {
            spacing: Nheko.paddingSmall
            Layout.fillWidth: true

            Label {
                id: encryptedText
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
                color: palette.text
                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth + 1
            }

            Button {
                visible: r.encryptionError == Olm.MissingSession || encryptionError == Olm.MissingSessionIndex
                text: qsTr("Request key")
                onClicked: room.requestKeyForEvent(eventId)
            }

        }

    }

    background: Rectangle {
        color: palette.alternateBase
        radius: fontMetrics.lineSpacing / 2 + 2 * Nheko.paddingMedium
        visible: !Settings.bubbles // the bubble in a bubble looks odd
    }
}
