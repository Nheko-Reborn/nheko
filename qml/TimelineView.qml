// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "components"
import "delegates"
import "device-verification"
import "emoji"
import "ui"
import "voip"
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Window 2.13
import im.nheko
import im.nheko

Item {
    id: timelineView

    property var room: null
    property var roomPreview: null
    property bool showBackButton: false

    clip: true

    Shortcut {
        sequence: StandardKey.Close

        onActivated: Rooms.resetCurrentRoom()
    }
    Label {
        anchors.centerIn: parent
        color: timelineRoot.palette.text
        font.pointSize: 24
        text: qsTr("No room open")
        visible: !room && !TimelineManager.isInitialSync && (!roomPreview || !roomPreview.roomid)
    }
    Spinner {
        anchors.centerIn: parent
        foreground: timelineRoot.palette.mid
        // height is somewhat arbitrary here... don't set width because width scales w/ height
        height: parent.height / 16
        running: TimelineManager.isInitialSync
        visible: TimelineManager.isInitialSync
        z: 3
    }
    ColumnLayout {
        id: timelineLayout
        anchors.fill: parent
        enabled: visible
        spacing: 0
        visible: room != null && !room.isSpace

        TopBar {
            showBackButton: timelineView.showBackButton
        }
        Rectangle {
            Layout.fillWidth: true
            color: Nheko.theme.separator
            height: 1
            z: 3
        }
        Rectangle {
            id: msgView
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: timelineRoot.palette.base

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
                        source: CallManager.isOnCall && CallManager.callType != Voip.VOICE ? "voip/VideoCall.qml" : ""

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
            color: Nheko.theme.separator
            height: 1
            z: 3
        }
        UploadBox {
        }
        NotificationWarning {
        }
        ReplyPopup {
        }
        MessageInput {
        }
    }
    ColumnLayout {
        id: preview

        property string avatarUrl: room ? room.roomAvatarUrl : (roomPreview ? roomPreview.roomAvatarUrl : "")
        property string roomId: room ? room.roomId : (roomPreview ? roomPreview.roomid : "")
        property string roomName: room ? room.roomName : (roomPreview ? roomPreview.roomName : "")
        property string roomTopic: room ? room.roomTopic : (roomPreview ? roomPreview.roomTopic : "")

        anchors.fill: parent
        anchors.margins: Nheko.paddingLarge
        enabled: visible
        spacing: Nheko.paddingLarge
        visible: room != null && room.isSpace || roomPreview != null

        Item {
            Layout.fillHeight: true
        }
        Avatar {
            Layout.alignment: Qt.AlignHCenter
            displayName: parent.roomName
            enabled: false
            height: 130
            roomid: parent.roomId
            url: parent.avatarUrl.replace("mxc://", "image://MxcImage/")
            width: 130
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Nheko.paddingMedium

            MatrixText {
                font.pixelSize: 24
                text: preview.roomName == "" ? qsTr("No preview available") : preview.roomName
            }
            ImageButton {
                ToolTip.text: qsTr("Settings")
                ToolTip.visible: hovered
                hoverEnabled: true
                image: ":/icons/icons/ui/settings.svg"
                visible: !!room

                onClicked: TimelineManager.openRoomSettings(room.roomId)
            }
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Nheko.paddingMedium
            visible: !!room

            MatrixText {
                cursorShape: Qt.PointingHandCursor
                text: qsTr("%1 member(s)").arg(room ? room.roomMemberCount : 0)
            }
            ImageButton {
                ToolTip.text: qsTr("View members of %1").arg(room.roomName)
                ToolTip.visible: hovered
                hoverEnabled: true
                image: ":/icons/icons/ui/people.svg"

                onClicked: TimelineManager.openRoomMembers(room)
            }
        }
        ScrollView {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.leftMargin: Nheko.paddingLarge
            Layout.rightMargin: Nheko.paddingLarge

            TextArea {
                background: null
                color: timelineRoot.palette.text
                horizontalAlignment: TextEdit.AlignHCenter
                readOnly: true
                selectByMouse: true
                text: TimelineManager.escapeEmoji(preview.roomTopic)
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.WordWrap

                onLinkActivated: Nheko.openLink(link)

                NhekoCursorShape {
                    anchors.fill: parent
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }
        }
        FlatButton {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("join the conversation")
            visible: roomPreview && !roomPreview.isInvite

            onClicked: Rooms.joinPreview(roomPreview.roomid)
        }
        FlatButton {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("accept invite")
            visible: roomPreview && roomPreview.isInvite

            onClicked: Rooms.acceptInvite(roomPreview.roomid)
        }
        FlatButton {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("decline invite")
            visible: roomPreview && roomPreview.isInvite

            onClicked: Rooms.declineInvite(roomPreview.roomid)
        }
        Item {
            Layout.preferredHeight: Math.ceil(fontMetrics.lineSpacing * 2)
            visible: room != null
        }
        Item {
            Layout.fillHeight: true
        }
    }
    ImageButton {
        id: backToRoomsButton
        ToolTip.text: qsTr("Back to room list")
        ToolTip.visible: hovered
        anchors.left: parent.left
        anchors.margins: Nheko.paddingMedium
        anchors.top: parent.top
        enabled: visible
        height: Nheko.avatarSize
        image: ":/icons/icons/ui/angle-arrow-left.svg"
        visible: (room == null || room.isSpace) && showBackButton
        width: Nheko.avatarSize

        onClicked: Rooms.resetCurrentRoom()
    }
    NhekoDropArea {
        anchors.fill: parent
        roomid: room ? room.roomId : ""
    }
    Connections {
        function onOpenReadReceiptsDialog(rr) {
            var dialog = readReceiptsDialog.createObject(timelineRoot, {
                    "readReceipts": rr,
                    "room": room
                });
            dialog.show();
            timelineRoot.destroyOnClose(dialog);
        }
        function onShowRawMessageDialog(rawMessage) {
            var dialog = rawMessageDialog.createObject(timelineRoot, {
                    "rawMessage": rawMessage
                });
            dialog.show();
            timelineRoot.destroyOnClose(dialog);
        }

        target: room
    }
}
