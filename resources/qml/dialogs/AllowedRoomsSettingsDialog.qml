// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../ui"
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0

ApplicationWindow {
    id: allowedDialog

    property var roomSettings

    minimumWidth: 340
    minimumHeight: 450
    width: 450
    height: 680
    palette: Nheko.colors
    color: Nheko.colors.window
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    title: qsTr("Allowed rooms settings")

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: roomSettingsDialog.close()
    }

    ColumnLayout {
        anchors.margins: Nheko.paddingMedium
        anchors.fill: parent
        spacing: 0


        MatrixText {
            text: qsTr("List of rooms that allow access to this room. Anyone who is in any of those rooms can join this room.")
            font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.1)
            Layout.fillWidth: true
            Layout.fillHeight: false
            color: Nheko.colors.text
            Layout.bottomMargin: Nheko.paddingMedium
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            id: view

            clip: true

            ScrollHelper {
                flickable: parent
                anchors.fill: parent
            }

            model: roomSettings.allowedRoomsModel
            spacing: 4
            cacheBuffer: 50

            delegate: RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                ColumnLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: model.name
                        color: Nheko.colors.text
                        textFormat: Text.PlainText
                    }

                    Text {
                        Layout.fillWidth: true
                        text: model.isParent ? qsTr("Parent community") : qsTr("Other room")
                        color: Nheko.colors.buttonText
                        textFormat: Text.PlainText
                    }
                }

                ToggleButton {
                    checked: model.allowed
                    Layout.alignment: Qt.AlignRight
                    onCheckedChanged: model.allowed = checked
                }
            }
        }

        Column{
            id: roomEntryCompleter
            Layout.fillWidth: true

            spacing: 1
            z: 5

            Completer {
                id: roomCompleter

                visible: roomEntry.text.length > 0
                width: parent.width
                roomId: allowedDialog.roomSettings.roomId
                completerName: "room"
                bottomToTop: true
                fullWidth: true
                avatarHeight: Nheko.avatarSize / 2
                avatarWidth: Nheko.avatarSize / 2
                centerRowContent: false
                rowMargin: 2
                rowSpacing: 2
            }

            MatrixTextField {
                id: roomEntry

                width: parent.width

                placeholderText: qsTr("Enter additional rooms not in the list yet...")

                color: Nheko.colors.text
                onTextEdited: {
                    roomCompleter.completer.searchString = text;
                }
                Keys.onPressed: {
                    if (event.key == Qt.Key_Up || event.key == Qt.Key_Backtab) {
                        event.accepted = true;
                        roomCompleter.up();
                    } else if (event.key == Qt.Key_Down || event.key == Qt.Key_Tab) {
                        event.accepted = true;
                        if (event.key == Qt.Key_Tab && (event.modifiers & Qt.ShiftModifier))
                        roomCompleter.up();
                        else
                        roomCompleter.down();
                    } else if (event.matches(StandardKey.InsertParagraphSeparator)) {
                        roomCompleter.finishCompletion();
                        event.accepted = true;
                    }
                }
            }

        }

        Connections {
            function onCompletionSelected(id) {
                console.log("selected: " + id);
                roomSettings.allowedRoomsModel.addRoom(id);
                roomEntry.clear();
            }

            function onCountChanged() {
                if (roomCompleter.count > 0 && (roomCompleter.currentIndex < 0 || roomCompleter.currentIndex >= roomCompleter.count))
                roomCompleter.currentIndex = 0;

            }

            target: roomCompleter
        }

    }

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        onAccepted: {
            roomSettings.applyAllowedFromModel();
            allowedDialog.close();
        }
        onRejected: allowedDialog.close()
    }

}
