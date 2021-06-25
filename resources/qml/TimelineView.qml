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

    ColumnLayout {
        id: timelineLayout

        visible: room != null && !room.isSpace
        enabled: visible
        anchors.fill: parent
        spacing: 0

        TopBar {
            showBackButton: timelineView.showBackButton
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
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

    ColumnLayout {
        visible: room != null && room.isSpace
        enabled: visible
        anchors.fill: parent
        anchors.margins: Nheko.paddingLarge
        spacing: Nheko.paddingLarge

        Avatar {
            url: room ? room.roomAvatarUrl.replace("mxc://", "image://MxcImage/") : ""
            displayName: room ? room.roomName : ""
            height: 130
            width: 130
            Layout.alignment: Qt.AlignHCenter
            enabled: false
        }

        MatrixText {
            text: room ? room.roomName : ""
            font.pixelSize: 24
            Layout.alignment: Qt.AlignHCenter
        }

        MatrixText {
            text: qsTr("%1 member(s)").arg(room ? room.roomMemberCount : 0)
            Layout.alignment: Qt.AlignHCenter
        }

        ScrollView {
            Layout.alignment: Qt.AlignHCenter
            width: timelineView.width - Nheko.paddingLarge * 2

            TextArea {
                text: TimelineManager.escapeEmoji(room ? room.roomTopic : "")
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

        Item {
            Layout.fillHeight: true
        }

    }

    ImageButton {
        id: backToRoomsButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: Nheko.paddingMedium
        width: Nheko.avatarSize
        height: Nheko.avatarSize
        visible: room != null && room.isSpace && showBackButton
        enabled: visible
        image: ":/icons/icons/ui/angle-pointing-to-left.png"
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Back to room list")
        onClicked: Rooms.resetCurrentRoom()
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
