// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./components"
import "./delegates"
import "./device-verification"
import "./emoji"
import "./ui"
import "./voip"
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Particles 2.15
import QtQuick.Window 2.13
import im.nheko 1.0
import im.nheko.EmojiModel 1.0

Item {
    id: timelineView

    property var room: null
    property var roomPreview: null
    property bool showBackButton: false
    property bool shouldEffectsRun: false
    required property PrivacyScreen privacyScreen
    clip: true

    onRoomChanged: if (room != null) room.triggerSpecialEffects()

    StickerPicker {
        id: emojiPopup

        colors: Nheko.colors
        emoji: true
    }

    // focus message input on key press, but not on Ctrl-C and such.
    Keys.onPressed: {
        if (event.text && event.key !== Qt.Key_Enter && event.key !== Qt.Key_Return && !topBar.searchHasFocus) {
            TimelineManager.focusMessageInput();
            room.input.setText(room.input.text + event.text);
        }
    }

    Shortcut {
        sequence: StandardKey.Close
        onActivated: Rooms.resetCurrentRoom()
    }

    Label {
        visible: !room && !TimelineManager.isInitialSync && (!roomPreview || !roomPreview.roomid)
        anchors.centerIn: parent
        text: qsTr("No room open")
        font.pointSize: 24
        color: Nheko.colors.text
    }

    Spinner {
        visible: TimelineManager.isInitialSync
        anchors.centerIn: parent
        foreground: Nheko.colors.mid
        running: TimelineManager.isInitialSync
        // height is somewhat arbitrary here... don't set width because width scales w/ height
        height: parent.height / 16
        z: 3
        opacity: hh.hovered ? 0.3 : 1

        Behavior on opacity {
            NumberAnimation { duration: 100; }
        }

        HoverHandler {
            id: hh
        }
    }

    ColumnLayout {
        id: timelineLayout

        visible: room != null && !room.isSpace
        enabled: visible
        anchors.fill: parent
        spacing: 0

        TopBar {
            id: topBar

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
                        implicitHeight: msgView.height - typingIndicator.height
                        searchString: topBar.searchString
                        Layout.fillWidth: true
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


        UploadBox {
        }

        MessageInputWarning {
            text: qsTr("You are about to notify the whole room")
            visible: (room && room.permissions.canPingRoom() && room.input.containsAtRoom)
        }

        MessageInputWarning {
            text: qsTr("The command /%1 is not recognized and will be sent as part of your message").arg(room ? room.input.currentCommand : "")
            visible: room ? room.input.containsInvalidCommand && !room.input.containsIncompleteCommand : false
        }

        MessageInputWarning {
            text: qsTr("/%1 looks like an incomplete command. To send it anyway, add a space to the end of your message.").arg(room ? room.input.currentCommand : "")
            visible: room ? room.input.containsIncompleteCommand : false
            bubbleColor: Nheko.theme.orange
        }

        ReplyPopup {
        }

        MessageInput {
        }

    }

    ColumnLayout {
        id: preview

        property string roomId: room ? room.roomId : (roomPreview ? roomPreview.roomid : "")
        property string roomName: room ? room.roomName : (roomPreview ? roomPreview.roomName : "")
        property string roomTopic: room ? room.roomTopic : (roomPreview ? roomPreview.roomTopic : "")
        property string avatarUrl: room ? room.roomAvatarUrl : (roomPreview ? roomPreview.roomAvatarUrl : "")
        property string reason: roomPreview ? roomPreview.reason : ""

        visible: room != null && room.isSpace || roomPreview != null
        enabled: visible
        anchors.fill: parent
        anchors.margins: Nheko.paddingLarge
        spacing: Nheko.paddingLarge

        Item {
            Layout.fillHeight: true
        }

        Avatar {
            url: parent.avatarUrl.replace("mxc://", "image://MxcImage/")
            roomid: parent.roomId
            displayName: parent.roomName
            height: 130
            width: 130
            Layout.alignment: Qt.AlignHCenter
            enabled: false
        }

        RowLayout {
            spacing: Nheko.paddingMedium
            Layout.alignment: Qt.AlignHCenter

            MatrixText {
                text: !roomPreview.isFetched ? qsTr("No preview available") : preview.roomName
                font.pixelSize: 24
            }

            ImageButton {
                image: ":/icons/icons/ui/settings.svg"
                visible: !!room
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Settings")
                onClicked: TimelineManager.openRoomSettings(room.roomId)
            }

        }

        RowLayout {
            visible: !!room
            spacing: Nheko.paddingMedium
            Layout.alignment: Qt.AlignHCenter

            MatrixText {
                text: qsTr("%n member(s)", "", room ? room.roomMemberCount : 0)
            }

            ImageButton {
                image: ":/icons/icons/ui/people.svg"
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("View members of %1").arg(room ? room.roomName : "")
                onClicked: TimelineManager.openRoomMembers(room)
            }

        }

        ScrollView {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.leftMargin: Nheko.paddingLarge
            Layout.rightMargin: Nheko.paddingLarge

            TextArea {
                text: roomPreview.isFetched ? TimelineManager.escapeEmoji(preview.roomTopic) : qsTr("This room is possibly inaccessible. If this room is private, you should remove it from this community.")
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

        FlatButton {
            visible: roomPreview && !roomPreview.isInvite
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("join the conversation")
            onClicked: Rooms.joinPreview(roomPreview.roomid)
        }

        FlatButton {
            visible: roomPreview && roomPreview.isInvite
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("accept invite")
            onClicked: Rooms.acceptInvite(roomPreview.roomid)
        }

        FlatButton {
            visible: roomPreview && roomPreview.isInvite
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("decline invite")
            onClicked: Rooms.declineInvite(roomPreview.roomid)
        }

        FlatButton {
            visible: !!room
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("leave")
            onClicked: TimelineManager.openLeaveRoomDialog(room.roomId)
        }

        ScrollView {
            id: reasonField
            property bool showReason: false

            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.leftMargin: Nheko.paddingLarge
            Layout.rightMargin: Nheko.paddingLarge
            visible: preview.reason !== "" && showReason

            TextArea {
                text: TimelineManager.escapeEmoji(preview.reason)
                wrapMode: TextEdit.WordWrap
                textFormat: TextEdit.RichText
                readOnly: true
                background: null
                selectByMouse: true
                color: Nheko.colors.text
                horizontalAlignment: TextEdit.AlignHCenter
            }

        }

        Button {
            id: showReasonButton

            Layout.alignment: Qt.AlignHCenter
            //Layout.fillWidth: true
            Layout.leftMargin: Nheko.paddingLarge
            Layout.rightMargin: Nheko.paddingLarge

            visible: preview.reason !== ""
            text: reasonField.showReason ? qsTr("Hide invite reason") : qsTr("Show invite reason")
            onClicked: {
                reasonField.showReason = !reasonField.showReason;
            }
        }

        Item {
            visible: room != null
            Layout.preferredHeight: Math.ceil(fontMetrics.lineSpacing * 2)
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
        visible: (room == null || room.isSpace) && showBackButton
        enabled: visible
        image: ":/icons/icons/ui/angle-arrow-left.svg"
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Back to room list")
        onClicked: Rooms.resetCurrentRoom()
    }

    TimelineEffects {
        id: timelineEffects

        anchors.fill: parent
    }

    NhekoDropArea {
        anchors.fill: parent
        roomid: room ? room.roomId : ""
    }

    Timer {
        id: effectsTimer
        onTriggered: shouldEffectsRun = false;
        interval: timelineEffects.maxLifespan
        repeat: false
        running: false
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
            var component = Qt.createComponent("qrc:/qml/dialogs/RawMessageDialog.qml")
            if (component.status == Component.Ready) {
                var dialog = component.createObject(timelineRoot, {
                    "rawMessage": rawMessage
                });
                dialog.show();
                timelineRoot.destroyOnClose(dialog);
            } else {
                console.error("Failed to create component: " + component.errorString());
            }
        }

        function onConfetti()
        {
            if (!Settings.fancyEffects)
                return

            shouldEffectsRun = true;
            timelineEffects.pulseConfetti()
            room.markSpecialEffectsDone()
        }

        function onConfettiDone()
        {
            if (!Settings.fancyEffects)
                return

            effectsTimer.restart();
        }

        function onRainfall()
        {
            if (!Settings.fancyEffects)
                return

            shouldEffectsRun = true;
            timelineEffects.pulseRainfall()
            room.markSpecialEffectsDone()
        }

        function onRainfallDone()
        {
            if (!Settings.fancyEffects)
                return

            effectsTimer.restart();
        }

        target: room
    }

}
