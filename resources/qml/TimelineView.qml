// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates"
import "./device-verification"
import "./emoji"
import "./voip"
import Qt.labs.platform 1.1 as Platform
import QtGraphicalEffects 1.0
import QtQuick 2.9
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import im.nheko 1.0
import im.nheko.EmojiModel 1.0
import im.nheko.RoomDirectoryModel 1.0

Item {
    id: timelineView

    property var room: null
    property bool showBackButton: false

    Label {
        visible: !room && !TimelineManager.isInitialSync
        anchors.centerIn: parent
        text: qsTr("No room open")
        font.pointSize: 24
        color: Nheko.colors.text
    }

    BusyIndicator {
        visible: running
        anchors.centerIn: parent
        running: TimelineManager.isInitialSync
        height: 200
        width: 200
        z: 3
    }

<<<<<<< HEAD
    Component {
        id: roomDirectoryComponent

        RoomDirectory {
        }
    }

    Shortcut {
        sequence: "Ctrl+K"
        onActivated: {
            var quickSwitch = quickSwitcherComponent.createObject(timelineRoot);
            TimelineManager.focusTimeline();
            quickSwitch.open();
        }
    }

    Platform.Menu {
        id: messageContextMenu

        property string eventId
        property string link
        property string text
        property int eventType
        property bool isEncrypted
        property bool isEditable
        property bool isSender

        function show(eventId_, eventType_, isSender_, isEncrypted_, isEditable_, link_, text_, showAt_) {
            eventId = eventId_;
            eventType = eventType_;
            isEncrypted = isEncrypted_;
            isEditable = isEditable_;
            isSender = isSender_;
            if (text_)
                text = text_;
            else
                text = "";
            if (link_)
                link = link_;
            else
                link = "";
            if (showAt_)
                open(showAt_);
            else
                open();
        }

        Platform.MenuItem {
            visible: messageContextMenu.text
            enabled: visible
            text: qsTr("Copy")
            onTriggered: Clipboard.text = messageContextMenu.text
        }

        Platform.MenuItem {
            visible: messageContextMenu.link
            enabled: visible
            text: qsTr("Copy link location")
            onTriggered: Clipboard.text = messageContextMenu.link
        }

        Platform.MenuItem {
            id: reactionOption

            visible: TimelineManager.timeline ? TimelineManager.timeline.permissions.canSend(MtxEvent.Reaction) : false
            text: qsTr("React")
            onTriggered: emojiPopup.show(null, function(emoji) {
                TimelineManager.queueReactionMessage(messageContextMenu.eventId, emoji);
            })
        }

        Platform.MenuItem {
            visible: TimelineManager.timeline ? TimelineManager.timeline.permissions.canSend(MtxEvent.TextMessage) : false
            text: qsTr("Reply")
            onTriggered: TimelineManager.timeline.replyAction(messageContextMenu.eventId)
        }

        Platform.MenuItem {
            visible: messageContextMenu.isEditable && (TimelineManager.timeline ? TimelineManager.timeline.permissions.canSend(MtxEvent.TextMessage) : false)
            enabled: visible
            text: qsTr("Edit")
            onTriggered: TimelineManager.timeline.editAction(messageContextMenu.eventId)
        }

        Platform.MenuItem {
            text: qsTr("Read receipts")
            onTriggered: TimelineManager.timeline.readReceiptsAction(messageContextMenu.eventId)
        }

        Platform.MenuItem {
            visible: messageContextMenu.eventType == MtxEvent.ImageMessage || messageContextMenu.eventType == MtxEvent.VideoMessage || messageContextMenu.eventType == MtxEvent.AudioMessage || messageContextMenu.eventType == MtxEvent.FileMessage || messageContextMenu.eventType == MtxEvent.Sticker || messageContextMenu.eventType == MtxEvent.TextMessage || messageContextMenu.eventType == MtxEvent.LocationMessage || messageContextMenu.eventType == MtxEvent.EmoteMessage || messageContextMenu.eventType == MtxEvent.NoticeMessage
            text: qsTr("Forward")
            onTriggered: {
                var forwardMess = forwardCompleterComponent.createObject(timelineRoot);
                forwardMess.setMessageEventId(messageContextMenu.eventId);
                forwardMess.open();
            }
        }

        Platform.MenuItem {
            text: qsTr("Mark as read")
        }

        Platform.MenuItem {
            text: qsTr("View raw message")
            onTriggered: TimelineManager.timeline.viewRawMessage(messageContextMenu.eventId)
        }

        Platform.MenuItem {
            // TODO(Nico): Fix this still being iterated over, when using keyboard to select options
            visible: messageContextMenu.isEncrypted
            enabled: visible
            text: qsTr("View decrypted raw message")
            onTriggered: TimelineManager.timeline.viewDecryptedRawMessage(messageContextMenu.eventId)
        }

        Platform.MenuItem {
            visible: (TimelineManager.timeline ? TimelineManager.timeline.permissions.canRedact() : false) || messageContextMenu.isSender
            text: qsTr("Remove message")
            onTriggered: TimelineManager.timeline.redactEvent(messageContextMenu.eventId)
        }

        Platform.MenuItem {
            visible: messageContextMenu.eventType == MtxEvent.ImageMessage || messageContextMenu.eventType == MtxEvent.VideoMessage || messageContextMenu.eventType == MtxEvent.AudioMessage || messageContextMenu.eventType == MtxEvent.FileMessage || messageContextMenu.eventType == MtxEvent.Sticker
            enabled: visible
            text: qsTr("Save as")
            onTriggered: TimelineManager.timeline.saveMedia(messageContextMenu.eventId)
        }

        Platform.MenuItem {
            visible: messageContextMenu.eventType == MtxEvent.ImageMessage || messageContextMenu.eventType == MtxEvent.VideoMessage || messageContextMenu.eventType == MtxEvent.AudioMessage || messageContextMenu.eventType == MtxEvent.FileMessage || messageContextMenu.eventType == MtxEvent.Sticker
            enabled: visible
            text: qsTr("Open in external program")
            onTriggered: TimelineManager.timeline.openMedia(messageContextMenu.eventId)
        }

        Platform.MenuItem {
            visible: messageContextMenu.eventId
            enabled: visible
            text: qsTr("Copy link to event")
            onTriggered: TimelineManager.timeline.copyLinkToEvent(messageContextMenu.eventId)
        }

    }
=======
    ColumnLayout {
        id: timelineLayout
>>>>>>> upstream/master

        visible: room != null
        anchors.fill: parent
        spacing: 0

        TopBar {
            showBackButton: timelineView.showBackButton
        }

<<<<<<< HEAD
        Connections {
            target: TimelineManager
            onNewDeviceVerificationRequest: {
                var dialog = deviceVerificationDialog.createObject(timelineRoot, {
                    "flow": flow
                });
                dialog.show();
            }
            onOpenProfile: {
                var userProfile = userProfileComponent.createObject(timelineRoot, {
                    "profile": profile
                });
                userProfile.show();
            }
        }

        Connections {
            target: TimelineManager.timeline
            onOpenRoomSettingsDialog: {
                var roomSettings = roomSettingsComponent.createObject(timelineRoot, {
                    "roomSettings": settings
                });
                roomSettings.show();
            }
        }

        Connections {
            target: CallManager
            onNewInviteState: {
                if (CallManager.haveCallInvite && Settings.mobileMode) {
                    var dialog = mobileCallInviteDialog.createObject(msgView);
                    dialog.open();
                }
            }
        }

        Connections {
            target: TimelineManager
            onShowPublicRooms: {
                console.debug("Roomdir from QML TimelineView");
                var win = roomDirectoryComponent.createObject(timelineRoot);
                win.show();
            }
        }

        Label {
            visible: !TimelineManager.timeline && !TimelineManager.isInitialSync
            anchors.centerIn: parent
            text: qsTr("No room open")
            font.pointSize: 24
            color: colors.text
        }

        BusyIndicator {
            visible: running
            anchors.centerIn: parent
            running: TimelineManager.isInitialSync
            height: 200
            width: 200
=======
        Rectangle {
            Layout.fillWidth: true
            height: 1
>>>>>>> upstream/master
            z: 3
            color: Nheko.theme.separator
        }

        Rectangle {
            id: msgView

            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Nheko.colors.base

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                StackLayout {
                    id: stackLayout

                    currentIndex: 0

                    Connections {
                        function onRoomChanged() {
                            stackLayout.currentIndex = 0;
                        }

                        target: timelineView
                    }

                    MessageView {
                        Layout.fillWidth: true
                        implicitHeight: msgView.height - typingIndicator.height
                    }

                    Loader {
                        source: CallManager.isOnCall && CallManager.callType != CallType.VOICE ? "voip/VideoCall.qml" : ""
                        onLoaded: TimelineManager.setVideoCallItem()
                    }

                }

                TypingIndicator {
                    id: typingIndicator
                }

            }

        }

        CallInviteBar {
            id: callInviteBar

            Layout.fillWidth: true
            z: 3
        }

        ActiveCallBar {
            Layout.fillWidth: true
            z: 3
        }

        Rectangle {
            Layout.fillWidth: true
            z: 3
            height: 1
            color: Nheko.theme.separator
        }

        ReplyPopup {
        }

        MessageInput {
        }

    }

    NhekoDropArea {
        anchors.fill: parent
        roomid: room ? room.roomId() : ""
    }

    Connections {
        target: room
        onOpenRoomSettingsDialog: {
            var roomSettings = roomSettingsComponent.createObject(timelineRoot, {
                "roomSettings": settings
            });
            roomSettings.show();
        }
    }

}
