import "./delegates"
import "./device-verification"
import "./emoji"
import "./voip"
import QtGraphicalEffects 1.0
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import im.nheko 1.0
import im.nheko.EmojiModel 1.0

Page {
    id: timelineRoot

    property var colors: currentActivePalette
    property var systemInactive
    property var inactiveColors: currentInactivePalette ? currentInactivePalette : systemInactive
    property int avatarSize: 40
    property real highlightHue: colors.highlight.hslHue
    property real highlightSat: colors.highlight.hslSaturation
    property real highlightLight: colors.highlight.hslLightness

    palette: colors

    FontMetrics {
        id: fontMetrics
    }

    EmojiPicker {
        id: emojiPopup

        width: 7 * 52 + 20
        height: 6 * 52
        colors: palette

        model: EmojiProxyModel {
            category: EmojiCategory.People

            sourceModel: EmojiModel {
            }

        }

    }

    Component {
        id: userProfileComponent

        UserProfile {
        }

    }

    Menu {
        id: messageContextMenu

        property string eventId
        property int eventType
        property bool isEncrypted

        function show(eventId_, eventType_, isEncrypted_, showAt_, position) {
            eventId = eventId_;
            eventType = eventType_;
            isEncrypted = isEncrypted_;
            if (position)
                popup(position, showAt_);
            else
                popup(showAt_);
        }

        modal: true

        MenuItem {
            text: qsTr("React")
            onClicked: emojiPopup.show(messageContextMenu.parent, function(emoji) {
                TimelineManager.queueReactionMessage(messageContextMenu.eventId, emoji);
            })
        }

        MenuItem {
            text: qsTr("Reply")
            onClicked: TimelineManager.timeline.replyAction(messageContextMenu.eventId)
        }

        MenuItem {
            text: qsTr("Read receipts")
            onTriggered: TimelineManager.timeline.readReceiptsAction(messageContextMenu.eventId)
        }

        MenuItem {
            text: qsTr("Mark as read")
        }

        MenuItem {
            text: qsTr("View raw message")
            onTriggered: TimelineManager.timeline.viewRawMessage(messageContextMenu.eventId)
        }

        MenuItem {
            // TODO(Nico): Fix this still being iterated over, when using keyboard to select options
            visible: messageContextMenu.isEncrypted
            height: visible ? implicitHeight : 0
            text: qsTr("View decrypted raw message")
            onTriggered: TimelineManager.timeline.viewDecryptedRawMessage(messageContextMenu.eventId)
        }

        MenuItem {
            text: qsTr("Remove message")
            onTriggered: TimelineManager.timeline.redactEvent(messageContextMenu.eventId)
        }

        MenuItem {
            visible: messageContextMenu.eventType == MtxEvent.ImageMessage || messageContextMenu.eventType == MtxEvent.VideoMessage || messageContextMenu.eventType == MtxEvent.AudioMessage || messageContextMenu.eventType == MtxEvent.FileMessage || messageContextMenu.eventType == MtxEvent.Sticker
            height: visible ? implicitHeight : 0
            text: qsTr("Save as")
            onTriggered: TimelineManager.timeline.saveMedia(messageContextMenu.eventId)
        }

    }

    Rectangle {
        anchors.fill: parent
        color: colors.window

        Component {
            id: deviceVerificationDialog

            DeviceVerification {
            }

        }

        Connections {
            target: TimelineManager
            onNewDeviceVerificationRequest: {
                var dialog = deviceVerificationDialog.createObject(timelineRoot, {
                    "flow": flow
                });
                dialog.show();
            }
        }

        Connections {
            target: TimelineManager.timeline
            onOpenProfile: {
                var userProfile = userProfileComponent.createObject(timelineRoot, {
                    "profile": profile
                });
                userProfile.show();
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
            z: 3
        }

        ColumnLayout {
            visible: TimelineManager.timeline != null
            anchors.fill: parent
            spacing: 0

            TopBar {
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                z: 3
                color: colors.mid
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: colors.base

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    StackLayout {
                        id: stackLayout

                        currentIndex: 0

                        Connections {
                            function onActiveTimelineChanged() {
                                stackLayout.currentIndex = 0;
                            }

                            target: TimelineManager
                        }

                        MessageView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }

                        Loader {
                            source: CallManager.isOnCall && CallManager.isVideo ? "voip/VideoCall.qml" : ""
                            onLoaded: TimelineManager.setVideoCallItem()
                        }

                    }

                    TypingIndicator {
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
                color: colors.mid
            }

            ReplyPopup {
            }

            MessageInput {
            }

        }

    }

    systemInactive: SystemPalette {
        colorGroup: SystemPalette.Disabled
    }

}
