// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import im.nheko

ApplicationWindow {
    id: allowedDialog

    property var roomSettings

    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 680
    minimumHeight: 450
    minimumWidth: 340
    modality: Qt.NonModal
    title: qsTr("Allowed rooms settings")
    width: 450

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onAccepted: {
            roomSettings.applyAllowedFromModel();
            allowedDialog.close();
        }
        onRejected: allowedDialog.close()
    }

    Shortcut {
        sequence: StandardKey.Cancel

        onActivated: roomSettingsDialog.close()
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: 0

        MatrixText {
            Layout.bottomMargin: Nheko.paddingMedium
            Layout.fillHeight: false
            Layout.fillWidth: true
            color: palette.text
            font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.1)
            text: qsTr("List of rooms that allow access to this room. Anyone who is in any of those rooms can join this room.")
        }
        ListView {
            id: view

            Layout.fillHeight: true
            Layout.fillWidth: true
            cacheBuffer: 50
            clip: true
            model: roomSettings.allowedRoomsModel
            spacing: 4

            delegate: RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                ColumnLayout {
                    Layout.fillWidth: true

                    Text {
                        Layout.fillWidth: true
                        color: palette.text
                        text: model.name
                        textFormat: Text.PlainText
                    }
                    Text {
                        Layout.fillWidth: true
                        color: palette.buttonText
                        text: model.isParent ? qsTr("Parent community") : qsTr("Other room")
                        textFormat: Text.PlainText
                    }
                }
                ToggleButton {
                    Layout.alignment: Qt.AlignRight
                    checked: model.allowed

                    onCheckedChanged: model.allowed = checked
                }
            }
        }
        Column {
            id: roomEntryCompleter

            Layout.fillWidth: true
            spacing: 1
            z: 5

            Completer {
                id: roomCompleter

                avatarHeight: Nheko.avatarSize / 2
                avatarWidth: Nheko.avatarSize / 2
                bottomToTop: true
                centerRowContent: false
                completerName: "room"
                fullWidth: true
                roomId: allowedDialog.roomSettings.roomId
                rowMargin: 2
                rowSpacing: 2
                visible: roomEntry.text.length > 0
                width: parent.width
            }
            MatrixTextField {
                id: roomEntry

                color: palette.text
                placeholderText: qsTr("Enter additional rooms not in the list yet...")
                width: parent.width

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
                onTextEdited: {
                    roomCompleter.completer.searchString = text;
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
}
