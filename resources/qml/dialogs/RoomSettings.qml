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

    minimumWidth: 340
    minimumHeight: 340
    palette: Nheko.colors
    color: Nheko.colors.window
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    title: qsTr("Room Settings")

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: roomSettingsDialog.close()
    }
    ScrollHelper {
        flickable: flickable
        anchors.fill: flickable
    }
    Flickable {
        id: flickable
        boundsBehavior: Flickable.StopAtBounds
        anchors.fill: parent
        clip: true
        flickableDirection: Flickable.VerticalFlick
        contentWidth: contentLayout1.width
        contentHeight: contentLayout1.height
        ColumnLayout {
            id: contentLayout1
            width: flickable.width
            spacing: Nheko.paddingMedium

            Avatar {
                Layout.topMargin: Nheko.paddingMedium
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
                wrapMode: Text.Wrap // somehow still doesn't wrap
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
                Label {
                    text: roomSettings.roomName
                    Layout.alignment: Qt.AlignHCenter
                    font.pixelSize: fontMetrics.font.pixelSize * 2
                    Layout.fillWidth: true
                    horizontalAlignment: TextEdit.AlignHCenter
                }

                Label {
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

            ImageButton {
                Layout.alignment: Qt.AlignHCenter
                image: ":/icons/icons/ui/edit.svg"
                visible: roomSettings.canChangeNameAndTopic
                onClicked: roomSettings.openEditModal()
            }

            TextArea {
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                Layout.leftMargin: Nheko.paddingLarge
                Layout.rightMargin: Nheko.paddingLarge

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

            GridLayout {
                columns: 2
                rowSpacing: Nheko.paddingMedium
                Layout.margins: Nheko.paddingMedium

                Label {
                    text: qsTr("SETTINGS")
                    font.bold: true
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
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

                Label {
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

                Label {
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

                Label {
                    text: qsTr("Sticker & Emote Settings")
                }

                Button {
                    text: qsTr("Change")
                    ToolTip.text: qsTr("Change what packs are enabled, remove packs or create new ones")
                    onClicked: TimelineManager.openImagePackSettings(roomSettings.roomId)
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("Hidden events")
                }

                HiddenEventsDialog {
                    id: hiddenEventsDialog
                    roomid: roomSettings.roomId
                    roomName: roomSettings.roomName
                }

                Button {
                    text: qsTr("Configure")
                    ToolTip.text: qsTr("Select events to hide in this room")
                    onClicked: hiddenEventsDialog.show()
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

                Label {
                    text: qsTr("INFO")
                    font.bold: true
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Internal ID")
                }

                Label {
                    text: roomSettings.roomId
                    font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 0.8)
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("Room Version")
                }

                Label {
                    text: roomSettings.roomVersion
                    font.pixelSize: fontMetrics.font.pixelSize
                    Layout.alignment: Qt.AlignRight
                }

            }
        }
    }
    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok
        onAccepted: close()
    }
}
