import "./delegates"
import "./device-verification"
import "./emoji"
import QtGraphicalEffects 1.0
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
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
            onClicked: emojiPopup.show(messageContextMenu.parent, messageContextMenu.eventId)
        }

        MenuItem {
            text: qsTr("Reply")
            onClicked: chat.model.replyAction(messageContextMenu.eventId)
        }

        MenuItem {
            text: qsTr("Read receipts")
            onTriggered: chat.model.readReceiptsAction(messageContextMenu.eventId)
        }

        MenuItem {
            text: qsTr("Mark as read")
        }

        MenuItem {
            text: qsTr("View raw message")
            onTriggered: chat.model.viewRawMessage(messageContextMenu.eventId)
        }

        MenuItem {
            visible: messageContextMenu.isEncrypted
            height: visible ? implicitHeight : 0
            text: qsTr("View decrypted raw message")
            onTriggered: chat.model.viewDecryptedRawMessage(messageContextMenu.eventId)
        }

        MenuItem {
            text: qsTr("Redact message")
            onTriggered: chat.model.redactEvent(messageContextMenu.eventId)
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
            function onNewDeviceVerificationRequest(flow, transactionId, userId, deviceId, isRequest) {
                var dialog = deviceVerificationDialog.createObject(timelineRoot, {
                    "flow": flow
                });
                dialog.show();
            }

            target: TimelineManager
        }

        Connections {
            function onOpenProfile(profile) {
                var userProfile = userProfileComponent.createObject(timelineRoot, {
                    "profile": profile
                });
                userProfile.show();
            }

            target: TimelineManager.timeline
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

            Rectangle {
                id: topBar

                Layout.fillWidth: true
                implicitHeight: topLayout.height + 16
                z: 3
                color: colors.window

                MouseArea {
                    anchors.fill: parent
                    onClicked: TimelineManager.openRoomSettings()
                }

                GridLayout {
                    //Layout.margins: 8

                    id: topLayout

                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 8
                    anchors.verticalCenter: parent.verticalCenter

                    ImageButton {
                        id: backToRoomsButton

                        Layout.column: 0
                        Layout.row: 0
                        Layout.rowSpan: 2
                        Layout.alignment: Qt.AlignVCenter
                        visible: TimelineManager.isNarrowView
                        image: ":/icons/icons/ui/angle-pointing-to-left.png"
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Back to room list")
                        onClicked: TimelineManager.backToRooms()
                    }

                    Avatar {
                        Layout.column: 1
                        Layout.row: 0
                        Layout.rowSpan: 2
                        Layout.alignment: Qt.AlignVCenter
                        width: avatarSize
                        height: avatarSize
                        url: chat.model ? chat.model.roomAvatarUrl.replace("mxc://", "image://MxcImage/") : ""
                        displayName: chat.model ? chat.model.roomName : qsTr("No room selected")

                        MouseArea {
                            anchors.fill: parent
                            onClicked: TimelineManager.openRoomSettings()
                        }

                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.column: 2
                        Layout.row: 0
                        color: colors.text
                        font.pointSize: fontMetrics.font.pointSize * 1.1
                        text: chat.model ? chat.model.roomName : qsTr("No room selected")

                        MouseArea {
                            anchors.fill: parent
                            onClicked: TimelineManager.openRoomSettings()
                        }

                    }

                    MatrixText {
                        Layout.fillWidth: true
                        Layout.column: 2
                        Layout.row: 1
                        Layout.maximumHeight: fontMetrics.lineSpacing * 2 // show 2 lines
                        clip: true
                        text: chat.model ? chat.model.roomTopic : ""
                    }

                    ImageButton {
                        id: roomOptionsButton

                        Layout.column: 3
                        Layout.row: 0
                        Layout.rowSpan: 2
                        Layout.alignment: Qt.AlignVCenter
                        image: ":/icons/icons/ui/vertical-ellipsis.png"
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Room options")
                        onClicked: roomOptionsMenu.popup(roomOptionsButton)

                        Menu {
                            id: roomOptionsMenu

                            MenuItem {
                                text: qsTr("Invite users")
                                onTriggered: TimelineManager.openInviteUsersDialog()
                            }

                            MenuItem {
                                text: qsTr("Members")
                                onTriggered: TimelineManager.openMemberListDialog()
                            }

                            MenuItem {
                                text: qsTr("Leave room")
                                onTriggered: TimelineManager.openLeaveRoomDialog()
                            }

                            MenuItem {
                                text: qsTr("Settings")
                                onTriggered: TimelineManager.openRoomSettings()
                            }

                        }

                    }

                }

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

                ListView {
                    id: chat

                    property int delegateMaxWidth: (Settings.timelineMaxWidth > 100 && (parent.width - Settings.timelineMaxWidth) > scrollbar.width * 2) ? Settings.timelineMaxWidth : (parent.width - scrollbar.width * 2)

                    cacheBuffer: 400
                    anchors.fill: parent
                    model: TimelineManager.timeline
                    boundsBehavior: Flickable.StopAtBounds
                    pixelAligned: true
                    spacing: 4
                    verticalLayoutDirection: ListView.BottomToTop
                    onCountChanged: {
                        if (atYEnd)
                            model.currentIndex = 0;

                    } // Mark last event as read, since we are at the bottom

                    ScrollHelper {
                        flickable: parent
                        anchors.fill: parent
                    }

                    Shortcut {
                        sequence: StandardKey.MoveToPreviousPage
                        onActivated: {
                            chat.contentY = chat.contentY - chat.height / 2;
                            chat.returnToBounds();
                        }
                    }

                    Shortcut {
                        sequence: StandardKey.MoveToNextPage
                        onActivated: {
                            chat.contentY = chat.contentY + chat.height / 2;
                            chat.returnToBounds();
                        }
                    }

                    Shortcut {
                        sequence: StandardKey.Cancel
                        onActivated: chat.model.reply = undefined
                    }

                    Shortcut {
                        sequence: "Alt+Up"
                        onActivated: chat.model.reply = chat.model.indexToId(chat.model.reply ? chat.model.idToIndex(chat.model.reply) + 1 : 0)
                    }

                    Shortcut {
                        sequence: "Alt+Down"
                        onActivated: {
                            var idx = chat.model.reply ? chat.model.idToIndex(chat.model.reply) - 1 : -1;
                            chat.model.reply = idx >= 0 ? chat.model.indexToId(idx) : undefined;
                        }
                    }

                    Component {
                        id: userProfileComponent

                        UserProfile {
                        }

                    }

                    section {
                        property: "section"
                    }

                    Component {
                        id: sectionHeader

                        Column {
                            property var modelData
                            property string section
                            property string nextSection

                            topPadding: 4
                            bottomPadding: 4
                            spacing: 8
                            visible: !!modelData
                            width: parent.width
                            height: (section.includes(" ") ? dateBubble.height + 8 + userName.height : userName.height) + 8

                            Label {
                                id: dateBubble

                                anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                                visible: section.includes(" ")
                                text: chat.model.formatDateSeparator(modelData.timestamp)
                                color: colors.text
                                height: fontMetrics.height * 1.4
                                width: contentWidth * 1.2
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter

                                background: Rectangle {
                                    radius: parent.height / 2
                                    color: colors.window
                                }

                            }

                            Row {
                                height: userName.height
                                spacing: 8

                                Avatar {
                                    width: avatarSize
                                    height: avatarSize
                                    url: chat.model.avatarUrl(modelData.userId).replace("mxc://", "image://MxcImage/")
                                    displayName: modelData.userName
                                    userid: modelData.userId

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: chat.model.openUserProfile(modelData.userId)
                                        cursorShape: Qt.PointingHandCursor
                                        propagateComposedEvents: true
                                    }

                                }

                                Label {
                                    id: userName

                                    text: TimelineManager.escapeEmoji(modelData.userName)
                                    color: TimelineManager.userColor(modelData.userId, colors.window)
                                    textFormat: Text.RichText

                                    MouseArea {
                                        anchors.fill: parent
                                        Layout.alignment: Qt.AlignHCenter
                                        onClicked: chat.model.openUserProfile(modelData.userId)
                                        cursorShape: Qt.PointingHandCursor
                                        propagateComposedEvents: true
                                    }

                                }

                            }

                        }

                    }

                    ScrollBar.vertical: ScrollBar {
                        id: scrollbar
                    }

                    delegate: Item {
                        id: wrapper

                        // This would normally be previousSection, but our model's order is inverted.
                        property bool sectionBoundary: (ListView.nextSection != "" && ListView.nextSection !== ListView.section) || model.index === chat.count - 1
                        property Item section

                        anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                        width: chat.delegateMaxWidth
                        height: section ? section.height + timelinerow.height : timelinerow.height
                        onSectionBoundaryChanged: {
                            if (sectionBoundary) {
                                var properties = {
                                    "modelData": model.dump,
                                    "section": ListView.section,
                                    "nextSection": ListView.nextSection
                                };
                                section = sectionHeader.createObject(wrapper, properties);
                            } else {
                                section.destroy();
                                section = null;
                            }
                        }

                        TimelineRow {
                            id: timelinerow

                            y: section ? section.y + section.height : 0
                        }

                        Connections {
                            function onMovementEnded() {
                                if (y + height + 2 * chat.spacing > chat.contentY + chat.height && y < chat.contentY + chat.height)
                                    chat.model.currentIndex = index;

                            }

                            target: chat
                        }

                    }

                    footer: BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        running: chat.model && chat.model.paginationInProgress
                        height: 50
                        width: 50
                        z: 3
                    }

                }

            }

            Item {
                implicitHeight: Math.max(fontMetrics.height * 1.2, typingDisplay.height)
                Layout.fillWidth: true

                Rectangle {
                    id: typingRect

                    color: colors.base
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: (chat.model && chat.model.typingUsers.length > 0) ? typingDisplay.height : 0
                    z: 3

                    Label {
                        id: typingDisplay

                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.bottom: parent.bottom
                        color: colors.text
                        text: chat.model ? chat.model.formatTypingUsers(chat.model.typingUsers, colors.base) : ""
                        textFormat: Text.RichText
                    }

                }

            }

            Rectangle {
                id: replyPopup

                Layout.fillWidth: true
                visible: chat.model && chat.model.reply
                // Height of child, plus margins, plus border
                implicitHeight: replyPreview.height + 10
                color: colors.window
                z: 3

                Reply {
                    id: replyPreview

                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.right: closeReplyButton.left
                    anchors.rightMargin: 20
                    anchors.bottom: parent.bottom
                    modelData: chat.model ? chat.model.getDump(chat.model.reply, chat.model.id) : {
                    }
                    userColor: TimelineManager.userColor(modelData.userId, colors.window)
                }

                ImageButton {
                    id: closeReplyButton

                    anchors.right: parent.right
                    anchors.rightMargin: 15
                    anchors.top: replyPreview.top
                    hoverEnabled: true
                    width: 16
                    height: 16
                    image: ":/icons/icons/ui/remove-symbol.png"
                    ToolTip.visible: closeReplyButton.hovered
                    ToolTip.text: qsTr("Close")
                    onClicked: chat.model.reply = undefined
                }

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

            Rectangle {
                color: colors.window
                Layout.fillWidth: true
                Layout.preferredHeight: textInput.height
                Layout.minimumHeight: 40

                RowLayout {
                    id: inputBar

                    anchors.fill: parent
                    spacing: 16

                    ImageButton {
                        Layout.alignment: Qt.AlignBottom
                        hoverEnabled: true
                        width: 22
                        height: 22
                        image: ":/icons/icons/ui/place-call.png"
                        Layout.topMargin: 8
                        Layout.bottomMargin: 8
                        Layout.leftMargin: 16
                    }

                    ImageButton {
                        Layout.alignment: Qt.AlignBottom
                        hoverEnabled: true
                        width: 22
                        height: 22
                        image: ":/icons/icons/ui/paper-clip-outline.png"
                        Layout.topMargin: 8
                        Layout.bottomMargin: 8
                    }

                    ScrollView {
                        id: textInput

                        Layout.alignment: Qt.AlignBottom
                        Layout.maximumHeight: Window.height / 4
                        Layout.fillWidth: true

                        TextArea {
                            placeholderText: qsTr("Write a message...")
                            placeholderTextColor: colors.buttonText
                            color: colors.text
                            wrapMode: TextEdit.Wrap

                            MouseArea {
                                // workaround for wrong cursor shape on some platforms
                                anchors.fill: parent
                                acceptedButtons: Qt.NoButton
                                cursorShape: Qt.IBeamCursor
                            }

                            background: Rectangle {
                                color: colors.window
                            }

                        }

                    }

                    ImageButton {
                        Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                        hoverEnabled: true
                        width: 22
                        height: 22
                        image: ":/icons/icons/ui/smile.png"
                        Layout.topMargin: 8
                        Layout.bottomMargin: 8
                    }

                    ImageButton {
                        Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                        hoverEnabled: true
                        width: 22
                        height: 22
                        image: ":/icons/icons/ui/cursor.png"
                        Layout.topMargin: 8
                        Layout.bottomMargin: 8
                        Layout.rightMargin: 16
                    }

                }

            }

        }

    }

    systemInactive: SystemPalette {
        colorGroup: SystemPalette.Disabled
    }

}
