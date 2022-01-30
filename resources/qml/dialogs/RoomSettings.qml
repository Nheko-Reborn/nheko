// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
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
    id: roomSettingsDialog

    property var roomSettings

    minimumWidth: 450
    minimumHeight: 680
    palette: Nheko.colors
    color: Nheko.colors.window
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    title: qsTr("Room Settings")

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: roomSettingsDialog.close()
    }

    ColumnLayout {
        id: contentLayout1

        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Avatar {
            url: roomSettings.roomAvatarUrl.replace("mxc://", "image://MxcImage/")
            roomid: roomSettings.roomId
            displayName: roomSettings.roomName
            height: 130
            width: 130
            Layout.alignment: Qt.AlignHCenter
            onClicked: {
                if (roomSettings.canChangeAvatar)
                    roomSettings.updateAvatar();

            }
        }

        Spinner {
            Layout.alignment: Qt.AlignHCenter
            visible: roomSettings.isLoading
            foreground: Nheko.colors.mid
            running: roomSettings.isLoading
        }

        Text {
            id: errorText

            color: "red"
            visible: opacity > 0
            opacity: 0
            Layout.alignment: Qt.AlignHCenter
        }

        SequentialAnimation {
            id: hideErrorAnimation

            running: false

            PauseAnimation {
                duration: 4000
            }

            NumberAnimation {
                target: errorText
                property: 'opacity'
                to: 0
                duration: 1000
            }

        }

        Connections {
            target: roomSettings
            function onDisplayError(errorMessage) {
                errorText.text = errorMessage;
                errorText.opacity = 1;
                hideErrorAnimation.restart();
            }
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter

            MatrixText {
                text: roomSettings.roomName
                font.pixelSize: fontMetrics.font.pixelSize * 2
                Layout.alignment: Qt.AlignHCenter
            }

            MatrixText {
                text: qsTr("%n member(s)", "", roomSettings.memberCount)
                Layout.alignment: Qt.AlignHCenter

                TapHandler {
                    onSingleTapped: TimelineManager.openRoomMembers(Rooms.getRoomById(roomSettings.roomId))
                }

                CursorShape {
                    cursorShape: Qt.PointingHandCursor
                    anchors.fill: parent
                }

            }

        }

        ImageButton {
            Layout.alignment: Qt.AlignHCenter
            image: ":/icons/icons/ui/edit.svg"
            visible: roomSettings.canChangeNameAndTopic
            onClicked: roomSettings.openEditModal()
        }

        ScrollView {
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.leftMargin: Nheko.paddingLarge
            Layout.rightMargin: Nheko.paddingLarge

            TextArea {
                text: TimelineManager.escapeEmoji(roomSettings.roomTopic)
                wrapMode: TextEdit.WordWrap
                textFormat: TextEdit.RichText
                readOnly: true
                background: null
                selectByMouse: true
                color: Nheko.colors.text
                horizontalAlignment: TextEdit.AlignHCenter
                onLinkActivated: Nheko.openLink(link)

                CursorShape {
                    anchors.fill: parent
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }

            }

        }

        GridLayout {
            columns: 2
            rowSpacing: Nheko.paddingMedium

            MatrixText {
                text: qsTr("SETTINGS")
                font.bold: true
            }

            Item {
                Layout.fillWidth: true
            }

            MatrixText {
                text: qsTr("Notifications")
                Layout.fillWidth: true
            }

            ComboBox {
                model: [qsTr("Muted"), qsTr("Mentions only"), qsTr("All messages")]
                currentIndex: roomSettings.notifications
                onActivated: {
                    roomSettings.changeNotifications(index);
                }
                Layout.fillWidth: true
            }

            MatrixText {
                text: qsTr("Room access")
                Layout.fillWidth: true
            }

            ComboBox {
                enabled: roomSettings.canChangeJoinRules
                model: {
                    let opts = [qsTr("Anyone and guests"), qsTr("Anyone"), qsTr("Invited users")];
                    if (roomSettings.supportsKnocking)
                        opts.push(qsTr("By knocking"));

                    if (roomSettings.supportsRestricted)
                        opts.push(qsTr("Restricted by membership in other rooms"));

                    return opts;
                }
                currentIndex: roomSettings.accessJoinRules
                onActivated: {
                    roomSettings.changeAccessRules(index);
                }
                Layout.fillWidth: true
            }

            MatrixText {
                text: qsTr("Encryption")
            }

            ToggleButton {
                id: encryptionToggle

                checked: roomSettings.isEncryptionEnabled
                onCheckedChanged: {
                    if (roomSettings.isEncryptionEnabled) {
                        checked = true;
                        return ;
                    }
                    confirmEncryptionDialog.open();
                }
                Layout.alignment: Qt.AlignRight
            }

            Platform.MessageDialog {
                id: confirmEncryptionDialog

                title: qsTr("End-to-End Encryption")
                text: qsTr("Encryption is currently experimental and things might break unexpectedly. <br>
                            Please take note that it can't be disabled afterwards.")
                modality: Qt.NonModal
                onAccepted: {
                    if (roomSettings.isEncryptionEnabled)
                        return ;

                    roomSettings.enableEncryption();
                }
                onRejected: {
                    encryptionToggle.checked = false;
                }
                buttons: Platform.MessageDialog.Ok | Platform.MessageDialog.Cancel
            }

            MatrixText {
                text: qsTr("Sticker & Emote Settings")
            }

            Button {
                text: qsTr("Change")
                ToolTip.text: qsTr("Change what packs are enabled, remove packs or create new ones")
                onClicked: TimelineManager.openImagePackSettings(roomSettings.roomId)
                Layout.alignment: Qt.AlignRight
            }

            Item {
                // for adding extra space between sections
                Layout.fillWidth: true
            }

            Item {
                // for adding extra space between sections
                Layout.fillWidth: true
            }

            MatrixText {
                text: qsTr("INFO")
                font.bold: true
            }

            Item {
                Layout.fillWidth: true
            }

            MatrixText {
                text: qsTr("Internal ID")
            }

            MatrixText {
                text: roomSettings.roomId
                font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 0.8)
                Layout.alignment: Qt.AlignRight
            }

            MatrixText {
                text: qsTr("Room Version")
            }

            MatrixText {
                text: roomSettings.roomVersion
                font.pixelSize: fontMetrics.font.pixelSize
                Layout.alignment: Qt.AlignRight
            }

        }

        DialogButtonBox {
            Layout.fillWidth: true
            standardButtons: DialogButtonBox.Ok
            onAccepted: close()
        }

    }

}
