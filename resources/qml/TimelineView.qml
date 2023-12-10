// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./components"
import "./emoji"
import "./ui"
import "./voip"
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import im.nheko

Item {
    id: timelineView

    required property PrivacyScreen privacyScreen
    property var room: null
    property var roomPreview: null
    property bool shouldEffectsRun: false
    property bool showBackButton: false

    clip: true

    // focus message input on key press, but not on Ctrl-C and such.
    Keys.onPressed: event => {
        if (event.text && event.key !== Qt.Key_Enter && event.key !== Qt.Key_Return && !topBar.searchHasFocus) {
            TimelineManager.focusMessageInput();
            room.input.setText(room.input.text + event.text);
        }
    }
    onRoomChanged: if (room != null)
        room.triggerSpecialEffects()

    StickerPicker {
        id: emojiPopup

        emoji: true
    }
    Shortcut {
        sequence: StandardKey.Close

        onActivated: Rooms.resetCurrentRoom()
    }
    Label {
        anchors.centerIn: parent
        font.pointSize: 24
        text: qsTr("No room open")
        visible: !room && !TimelineManager.isInitialSync && (!roomPreview || !roomPreview.roomid)
    }
    Spinner {
        anchors.centerIn: parent
        foreground: palette.mid
        // height is somewhat arbitrary here... don't set width because width scales w/ height
        height: parent.height / 16
        opacity: hh.hovered ? 0.3 : 1
        running: TimelineManager.isInitialSync
        visible: TimelineManager.isInitialSync
        z: 3

        Behavior on opacity  {
            NumberAnimation {
                duration: 100
            }
        }

        HoverHandler {
            id: hh

        }
    }
    ColumnLayout {
        id: timelineLayout

        anchors.fill: parent
        enabled: visible
        spacing: 0
        visible: room != null && !room.isSpace

        TopBar {
            id: topBar

            showBackButton: timelineView.showBackButton
        }
        Rectangle {
            Layout.fillWidth: true
            color: Nheko.theme.separator
            implicitHeight: 1
            z: 3
        }
        Rectangle {
            id: msgView

            Layout.fillHeight: true
            Layout.fillWidth: true
            color: palette.base

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
                        searchString: topBar.searchString
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
            implicitHeight: 1
            z: 3
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
            bubbleColor: Nheko.theme.orange
            text: qsTr("/%1 looks like an incomplete command. To send it anyway, add a space to the end of your message.").arg(room ? room.input.currentCommand : "")
            visible: room ? room.input.containsIncompleteCommand : false
        }
        ReplyPopup {
        }
        MessageInput {
        }
    }
    ColumnLayout {
        id: preview

        property string avatarUrl: room ? room.roomAvatarUrl : (roomPreview ? roomPreview.roomAvatarUrl : "")
        property string reason: roomPreview ? roomPreview.reason : ""
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
            implicitHeight: 130
            roomid: parent.roomId
            url: parent.avatarUrl.replace("mxc://", "image://MxcImage/")
            implicitWidth: 130
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Nheko.paddingMedium
            Layout.fillWidth: true

            MatrixText {
                Layout.preferredWidth: implicitWidth
                horizontalAlignment: TextEdit.AlignRight
                font.pixelSize: 24
                text: (!room && !(roomPreview?.isFetched ?? false)) ? qsTr("No preview available") : preview.roomName
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
            Layout.fillWidth: true

            MatrixText {
                Layout.preferredWidth: implicitWidth
                text: qsTr("%n member(s)", "", room ? room.roomMemberCount : 0)
            }
            ImageButton {
                ToolTip.text: qsTr("View members of %1").arg(room ? room.roomName : "")
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
            Layout.maximumHeight: timelineView.height / 3

            TextArea {
                background: null
                horizontalAlignment: TextEdit.AlignHCenter
                readOnly: true
                selectByMouse: true
                text: (room || (roomPreview?.isFetched ?? false)) ? TimelineManager.escapeEmoji(preview.roomTopic) : qsTr("This room is possibly inaccessible. If this room is private, you should remove it from this community.")
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
        FlatButton {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("leave")
            visible: !!room

            onClicked: TimelineManager.openLeaveRoomDialog(room.roomId)
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            spacing: Nheko.paddingMedium
            visible: roomPreview && roomPreview.isInvite && reasonField.showReason

            MatrixText {
                text: qsTr("Invited by %1 (%2)").arg(TimelineManager.escapeEmoji(inviterAvatar.displayName)).arg(TimelineManager.escapeEmoji(TimelineManager.htmlEscape(inviterAvatar.userid)))
            }
            Avatar {
                id: inviterAvatar

                Layout.alignment: Qt.AlignHCenter
                displayName: roomPreview?.inviterDisplayName ?? ""
                enabled: true
                implicitHeight: 48
                roomid: preview.roomId
                url: (roomPreview?.inviterAvatarUrl ?? "").replace("mxc://", "image://MxcImage/")
                userid: roomPreview?.inviterUserId ?? ""
                implicitWidth: 48

                onClicked: TimelineManager.openGlobalUserProfile(roomPreview.inviterUserId)
            }
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
                background: null
                horizontalAlignment: TextEdit.AlignHCenter
                readOnly: true
                selectByMouse: true
                text: TimelineManager.escapeEmoji(preview.reason)
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.WordWrap
            }
        }
        Button {
            id: showReasonButton

            Layout.alignment: Qt.AlignHCenter
            //Layout.fillWidth: true
            Layout.leftMargin: Nheko.paddingLarge
            Layout.rightMargin: Nheko.paddingLarge
            text: reasonField.showReason ? qsTr("Hide invite reason") : qsTr("Show invite reason")
            visible: roomPreview && roomPreview.isInvite

            onClicked: {
                reasonField.showReason = !reasonField.showReason;
            }
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
    TimelineEffects {
        id: timelineEffects

        anchors.fill: parent
        shouldEffectsRun: timelineView.shouldEffectsRun
    }
    NhekoDropArea {
        anchors.fill: parent
        roomid: room ? room.roomId : ""
    }
    Timer {
        id: effectsTimer

        interval: timelineEffects.maxLifespan
        repeat: false
        running: false

        onTriggered: {
            timelineEffects.removeParticles()
            shouldEffectsRun = false
        }
    }
    Connections {
        function onConfetti() {
            if (!Settings.fancyEffects)
                return;
            shouldEffectsRun = true;
            timelineEffects.pulseConfetti();
            room.markSpecialEffectsDone();
        }
        function onConfettiDone() {
            if (!Settings.fancyEffects)
                return;
            effectsTimer.restart();
        }
        function onOpenReadReceiptsDialog(rr) {
            var dialog = readReceiptsDialog.createObject(timelineRoot, {
                    "readReceipts": rr,
                    "room": room
                });
            dialog.show();
            timelineRoot.destroyOnClose(dialog);
        }
        function onRainfall() {
            if (!Settings.fancyEffects)
                return;
            shouldEffectsRun = true;
            timelineEffects.pulseRainfall();
            room.markSpecialEffectsDone();
        }
        function onRainfallDone() {
            if (!Settings.fancyEffects)
                return;
            effectsTimer.restart();
        }
        function onShowRawMessageDialog(rawMessage) {
            var component = Qt.createComponent("qrc:/resources/qml/dialogs/RawMessageDialog.qml");
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

        target: room
    }
}
