// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./components"
import "./delegates"
import "./emoji"
import "./ui"
import "./dialogs"
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0

Item {
    id: chatRoot

    property int availableWidth: width
    property int padding: Nheko.paddingMedium
    property string searchString: ""

    // HACK: https://bugreports.qt.io/browse/QTBUG-83972, qtwayland cannot auto hide menu
    Connections {
        function onHideMenu() {
            messageContextMenu.close();
            replyContextMenu.close();
        }

        target: MainWindow
    }
    ScrollBar {
        id: scrollbar

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.top: parent.top
        parent: chat.parent
    }
    ListView {
        id: chat

        property int delegateMaxWidth: ((Settings.timelineMaxWidth > 100 && Settings.timelineMaxWidth < chatRoot.availableWidth) ? Settings.timelineMaxWidth : chatRoot.availableWidth) - chatRoot.padding * 2 - (scrollbar.interactive ? scrollbar.width : 0)
        readonly property alias filteringInProgress: filteredTimeline.filteringInProgress

        ScrollBar.vertical: scrollbar
        anchors.fill: parent
        anchors.rightMargin: scrollbar.interactive ? scrollbar.width : 0
        // reuseItems still has a few bugs, see https://bugreports.qt.io/browse/QTBUG-95105 https://bugreports.qt.io/browse/QTBUG-95107
        //onModelChanged: if (room) room.sendReset()
        //reuseItems: true
        boundsBehavior: Flickable.StopAtBounds
        displayMarginBeginning: height / 2
        displayMarginEnd: height / 2
        model: (filteredTimeline.filterByThread || filteredTimeline.filterByContent) ? filteredTimeline : room
        //pixelAligned: true
        spacing: 2
        verticalLayoutDirection: ListView.BottomToTop

        delegate: Item {
            id: wrapper

            required property string blurhash
            required property string body
            required property string callType
            required property var day
            required property string duration
            required property int encryptionError
            required property string eventId
            required property string filename
            required property string filesize
            required property string formattedBody
            required property int index
            required property bool isEditable
            required property bool isEdited
            required property bool isEncrypted
            required property bool isOnlyEmoji
            required property bool isSender
            required property bool isStateEvent
            required property int notificationlevel
            required property int originalWidth
            property var previousMessageDay: (index + 1) >= chat.count ? 0 : chat.model.dataByIndex(index + 1, Room.Day)
            property bool previousMessageIsStateEvent: (index + 1) >= chat.count ? true : chat.model.dataByIndex(index + 1, Room.IsStateEvent)
            property string previousMessageUserId: (index + 1) >= chat.count ? "" : chat.model.dataByIndex(index + 1, Room.UserId)
            required property double proportionalHeight
            required property var reactions
            required property int relatedEventCacheBuster
            required property string replyTo
            required property string roomName
            required property string roomTopic
            property bool scrolledToThis: eventId === room.scrollTarget && (y + height > chat.y + chat.contentY && y < chat.y + chat.height + chat.contentY)
            required property int status
            required property string threadId
            required property string thumbnailUrl
            required property var timestamp
            required property int trustlevel
            required property int type
            required property string typeString
            required property string url
            required property string userId
            required property string userName

            ListView.delayRemove: true
            anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
            height: (section.item?.height ?? 0) + timelinerow.height
            width: chat.delegateMaxWidth

            Loader {
                id: section

                property var day: wrapper.day
                property bool isSender: wrapper.isSender
                property bool isStateEvent: wrapper.isStateEvent
                property int parentWidth: parent.width
                property var previousMessageDay: wrapper.previousMessageDay
                property bool previousMessageIsStateEvent: wrapper.previousMessageIsStateEvent
                property string previousMessageUserId: wrapper.previousMessageUserId
                property date timestamp: wrapper.timestamp
                property string userId: wrapper.userId
                property string userName: wrapper.userName

                active: previousMessageUserId !== userId || previousMessageDay !== day || previousMessageIsStateEvent !== isStateEvent
                //asynchronous: true
                sourceComponent: sectionHeader
                visible: status == Loader.Ready
                z: 4
            }
            TimelineRow {
                id: timelinerow

                blurhash: wrapper.blurhash
                body: wrapper.body
                callType: wrapper.callType
                duration: wrapper.duration
                encryptionError: wrapper.encryptionError
                eventId: chat.model, wrapper.eventId
                filename: wrapper.filename
                filesize: wrapper.filesize
                formattedBody: wrapper.formattedBody
                index: wrapper.index
                isEditable: wrapper.isEditable
                isEdited: wrapper.isEdited
                isEncrypted: wrapper.isEncrypted
                isOnlyEmoji: wrapper.isOnlyEmoji
                isSender: wrapper.isSender
                isStateEvent: wrapper.isStateEvent
                notificationlevel: wrapper.notificationlevel
                originalWidth: wrapper.originalWidth
                proportionalHeight: wrapper.proportionalHeight
                reactions: wrapper.reactions
                relatedEventCacheBuster: wrapper.relatedEventCacheBuster
                replyTo: wrapper.replyTo
                roomName: wrapper.roomName
                roomTopic: wrapper.roomTopic
                status: wrapper.status
                threadId: wrapper.threadId
                thumbnailUrl: wrapper.thumbnailUrl
                timestamp: wrapper.timestamp
                trustlevel: wrapper.trustlevel
                type: chat.model, wrapper.type
                typeString: wrapper.typeString
                url: wrapper.url
                userId: wrapper.userId
                userName: wrapper.userName
                width: wrapper.width
                y: section.visible && section.active ? section.y + section.height : 0

                background: Rectangle {
                    id: scrollHighlight

                    color: palette.highlight
                    enabled: false
                    opacity: 0
                    visible: true
                    z: 1

                    states: State {
                        name: "revealed"
                        when: wrapper.scrolledToThis
                    }
                    transitions: Transition {
                        from: ""
                        to: "revealed"

                        SequentialAnimation {
                            PropertyAnimation {
                                duration: 500
                                easing.type: Easing.InOutQuad
                                from: 0
                                properties: "opacity"
                                target: scrollHighlight
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 500
                                easing.type: Easing.InOutQuad
                                from: 1
                                properties: "opacity"
                                target: scrollHighlight
                                to: 0
                            }
                            ScriptAction {
                                script: room.eventShown()
                            }
                        }
                    }
                }

                onHoveredChanged: {
                    if (!Settings.mobileMode && hovered) {
                        if (!messageActions.hovered) {
                            messageActions.attached = timelinerow;
                            messageActions.model = timelinerow;
                        }
                    }
                }
            }
            Connections {
                function onMovementEnded() {
                    if (y + height + 2 * chat.spacing > chat.contentY + chat.height && y < chat.contentY + chat.height)
                        chat.model.currentIndex = index;
                }

                target: chat
            }
        }
        footer: Item {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.margins: Nheko.paddingLarge
            // hacky, but works
            height: loadingSpinner.height + 2 * Nheko.paddingLarge
            visible: (room && room.paginationInProgress) || chat.filteringInProgress

            Spinner {
                id: loadingSpinner

                anchors.centerIn: parent
                anchors.margins: Nheko.paddingLarge
                foreground: palette.mid
                running: (room && room.paginationInProgress) || chat.filteringInProgress
                z: 3
            }
        }

        Window.onActiveChanged: readTimer.running = Window.active
        onCountChanged: {
            // Mark timeline as read
            if (atYEnd && room)
                model.currentIndex = 0;
        }

        TimelineFilter {
            id: filteredTimeline

            filterByContent: chatRoot.searchString
            filterByThread: room ? room.thread : ""
            source: room
        }
        Control {
            id: messageActions

            property Item attached: null
            // use comma to update on scroll
            property var attachedPos: chat.contentY, attached ? chat.mapFromItem(attached, attached ? attached.width - width : 0, -height) : null
            property alias model: row.model

            hoverEnabled: true
            padding: Nheko.paddingSmall
            visible: Settings.buttonsInTimeline && !!attached && (attached.hovered || hovered)
            x: attached ? attachedPos.x : 0
            y: attached ? attachedPos.y + Nheko.paddingSmall : 0
            z: 10

            background: Rectangle {
                border.color: palette.buttonText
                border.width: 1
                color: palette.window
                radius: padding
            }
            contentItem: RowLayout {
                id: row

                property var model

                spacing: messageActions.padding

                Repeater {
                    model: Settings.recentReactions
                    visible: room ? room.permissions.canSend(MtxEvent.Reaction) : false

                    delegate: AbstractButton {
                        id: button

                        property color buttonTextColor: palette.buttonText
                        property color highlightColor: palette.highlight
                        required property string modelData
                        property bool showImage: modelData.startsWith("mxc://")

                        //Layout.preferredHeight: fontMetrics.height
                        Layout.alignment: Qt.AlignBottom
                        focusPolicy: Qt.NoFocus
                        height: showImage ? 16 : buttonText.implicitHeight
                        implicitHeight: showImage ? 16 : buttonText.implicitHeight
                        implicitWidth: showImage ? 16 : buttonText.implicitWidth
                        width: showImage ? 16 : buttonText.implicitWidth

                        onClicked: {
                            room.input.reaction(row.model.eventId, modelData);
                            TimelineManager.focusMessageInput();
                        }

                        Label {
                            id: buttonText

                            anchors.centerIn: parent
                            color: button.hovered ? button.highlightColor : button.buttonTextColor
                            font.family: Settings.emojiFont
                            horizontalAlignment: Text.AlignHCenter
                            padding: 0
                            text: button.modelData
                            verticalAlignment: Text.AlignVCenter
                            visible: !button.showImage
                        }
                        Image {
                            id: buttonImg

                            // Workaround, can't get icon.source working for now...
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            source: button.showImage ? (button.modelData.replace("mxc://", "image://MxcImage/") + "?scale") : ""
                            sourceSize.height: button.height
                            sourceSize.width: button.width
                        }
                        CursorShape {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                        }
                        Ripple {
                            color: Qt.rgba(buttonTextColor.r, buttonTextColor.g, buttonTextColor.b, 0.5)
                        }
                    }
                }
                ImageButton {
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Edit")
                    ToolTip.visible: hovered
                    buttonTextColor: palette.buttonText
                    hoverEnabled: true
                    image: ":/icons/icons/ui/edit.svg"
                    visible: !!row.model && row.model.isEditable
                    width: 16

                    onClicked: {
                        if (row.model.isEditable)
                            room.edit = row.model.eventId;
                    }
                }
                ImageButton {
                    id: reactButton

                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("React")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/smile-add.svg"
                    visible: room ? room.permissions.canSend(MtxEvent.Reaction) : false
                    width: 16

                    onClicked: emojiPopup.visible ? emojiPopup.close() : emojiPopup.show(reactButton, room.roomId, function (plaintext, markdown) {
                            var event_id = row.model ? row.model.eventId : "";
                            room.input.reaction(event_id, plaintext);
                            TimelineManager.focusMessageInput();
                        })
                }
                ImageButton {
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: (row.model && row.model.threadId) ? qsTr("Reply in thread") : qsTr("New thread")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: (row.model && row.model.threadId) ? ":/icons/icons/ui/thread.svg" : ":/icons/icons/ui/new-thread.svg"
                    visible: room ? room.permissions.canSend(MtxEvent.TextMessage) : false
                    width: 16

                    onClicked: room.thread = (row.model.threadId || row.model.eventId)
                }
                ImageButton {
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Reply")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/reply.svg"
                    visible: room ? room.permissions.canSend(MtxEvent.TextMessage) : false
                    width: 16

                    onClicked: room.reply = row.model.eventId
                }
                ImageButton {
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Go to message")
                    ToolTip.visible: hovered
                    buttonTextColor: palette.buttonText
                    hoverEnabled: true
                    image: ":/icons/icons/ui/go-to.svg"
                    visible: !!row.model && filteredTimeline.filterByContent
                    width: 16

                    onClicked: {
                        topBar.searchString = "";
                        room.showEvent(row.model.eventId);
                    }
                }
                ImageButton {
                    id: optionsButton

                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Options")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/options.svg"
                    width: 16

                    onClicked: messageContextMenu.show(row.model.eventId, row.model.threadId, row.model.type, row.model.isSender, row.model.isEncrypted, row.model.isEditable, "", row.model.body, optionsButton)
                }
            }
        }
        Shortcut {
            sequence: StandardKey.MoveToPreviousPage

            onActivated: {
                chat.contentY = chat.contentY - chat.height * 0.9;
                chat.returnToBounds();
            }
        }
        Shortcut {
            sequence: StandardKey.MoveToNextPage

            onActivated: {
                chat.contentY = chat.contentY + chat.height * 0.9;
                chat.returnToBounds();
            }
        }
        Shortcut {
            sequence: StandardKey.Cancel

            onActivated: {
                if (room.input.uploads.length > 0)
                    room.input.declineUploads();
                else if (room.reply)
                    room.reply = undefined;
                else if (room.edit)
                    room.edit = undefined;
                else
                    room.thread = undefined;
                TimelineManager.focusMessageInput();
            }
        }

        // These shortcuts use the room timeline because switching to threads and out is annoying otherwise.
        // Better solution welcome.
        Shortcut {
            sequence: "Alt+Up"

            onActivated: room.reply = room.indexToId(room.reply ? room.idToIndex(room.reply) + 1 : 0)
        }
        Shortcut {
            sequence: "Alt+Down"

            onActivated: {
                var idx = room.reply ? room.idToIndex(room.reply) - 1 : -1;
                room.reply = idx >= 0 ? room.indexToId(idx) : null;
            }
        }
        Shortcut {
            sequence: "Alt+F"

            onActivated: {
                if (room.reply) {
                    var forwardMess = forwardCompleterComponent.createObject(timelineRoot);
                    forwardMess.setMessageEventId(room.reply);
                    forwardMess.open();
                    room.reply = null;
                    timelineRoot.destroyOnClose(forwardMess);
                }
            }
        }
        Shortcut {
            sequence: "Ctrl+E"

            onActivated: {
                room.edit = room.reply;
            }
        }
        Timer {
            id: readTimer

            interval: 1000

            // force current read index to update
            onTriggered: {
                if (room)
                    room.setCurrentIndex(room.currentIndex);
            }
        }
        Component {
            id: sectionHeader

            Column {
                bottomPadding: Settings.bubbles ? (isSender && previousMessageDay == day ? 0 : 2) : 3
                spacing: 8
                topPadding: userName_.visible ? 4 : 0
                visible: (previousMessageUserId !== userId || previousMessageDay !== day || isStateEvent !== previousMessageIsStateEvent)
                width: parentWidth

                Label {
                    id: dateBubble

                    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                    color: palette.text
                    height: Math.round(fontMetrics.height * 1.4)
                    horizontalAlignment: Text.AlignHCenter
                    text: room ? room.formatDateSeparator(timestamp) : ""
                    verticalAlignment: Text.AlignVCenter
                    visible: room && previousMessageDay !== day
                    width: contentWidth * 1.2

                    background: Rectangle {
                        color: palette.window
                        radius: parent.height / 2
                    }
                }
                Row {
                    id: userInfo

                    property int remainingWidth: chat.delegateMaxWidth - spacing - messageUserAvatar.width

                    height: userName_.height
                    spacing: 8
                    visible: !isStateEvent && (!isSender || !Settings.bubbles)

                    Avatar {
                        id: messageUserAvatar

                        ToolTip.delay: Nheko.tooltipDelay
                        ToolTip.text: userid
                        ToolTip.visible: messageUserAvatar.hovered
                        displayName: userName
                        height: Nheko.avatarSize * (Settings.smallAvatars ? 0.5 : 1)
                        url: !room ? "" : room.avatarUrl(userId).replace("mxc://", "image://MxcImage/")
                        userid: userId
                        width: Nheko.avatarSize * (Settings.smallAvatars ? 0.5 : 1)

                        onClicked: room.openUserProfile(userId)
                    }
                    Connections {
                        function onRoomAvatarUrlChanged() {
                            messageUserAvatar.url = room.avatarUrl(userId).replace("mxc://", "image://MxcImage/");
                        }
                        function onScrollToIndex(index) {
                            chat.positionViewAtIndex(index, ListView.Center);
                        }

                        target: room
                    }
                    AbstractButton {
                        id: userNameButton

                        ToolTip.delay: Nheko.tooltipDelay
                        ToolTip.text: userId
                        ToolTip.visible: hovered
                        leftInset: 0
                        leftPadding: 0
                        rightInset: 0
                        rightPadding: 0

                        contentItem: Label {
                            id: userName_

                            color: TimelineManager.userColor(userId, palette.base)
                            text: TimelineManager.escapeEmoji(userNameTextMetrics.elidedText)
                            textFormat: Text.RichText
                        }

                        onClicked: room.openUserProfile(userId)

                        TextMetrics {
                            id: userNameTextMetrics

                            elide: Text.ElideRight
                            elideWidth: userInfo.remainingWidth - Math.min(statusMsg.implicitWidth, userInfo.remainingWidth / 3)
                            text: userName
                        }
                        CursorShape {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                        }
                    }
                    Label {
                        id: statusMsg

                        property string userStatus: Presence.userStatus(userId)

                        ToolTip.delay: Nheko.tooltipDelay
                        ToolTip.text: qsTr("%1's status message").arg(userName)
                        ToolTip.visible: statusMsgHoverHandler.hovered
                        anchors.baseline: userNameButton.baseline
                        color: palette.buttonText
                        elide: Text.ElideRight
                        font.italic: true
                        font.pointSize: Math.floor(fontMetrics.font.pointSize * 0.8)
                        text: userStatus.replace(/\n/g, " ")
                        textFormat: Text.PlainText
                        width: Math.min(implicitWidth, userInfo.remainingWidth - userName_.width - parent.spacing)

                        HoverHandler {
                            id: statusMsgHoverHandler

                        }
                        Connections {
                            function onPresenceChanged(id) {
                                if (id == userId)
                                    statusMsg.userStatus = Presence.userStatus(userId);
                            }

                            target: Presence
                        }
                    }
                }
            }
        }
    }
    Platform.Menu {
        id: messageContextMenu

        property string eventId
        property int eventType
        property bool isEditable
        property bool isEncrypted
        property bool isSender
        property string link
        property string text
        property string threadId

        function show(eventId_, threadId_, eventType_, isSender_, isEncrypted_, isEditable_, link_, text_, showAt_) {
            eventId = eventId_;
            threadId = threadId_;
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

        Component {
            id: removeReason

            InputDialog {
                id: removeReasonDialog

                property string eventId

                prompt: qsTr("Enter reason for removal or hit enter for no reason:")
                title: qsTr("Reason for removal")

                onAccepted: function (text) {
                    room.redactEvent(eventId, text);
                }
            }
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("Go to &message")
            visible: filteredTimeline.filterByContent

            onTriggered: function () {
                topBar.searchString = "";
                room.showEvent(messageContextMenu.eventId);
            }
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Copy")
            visible: messageContextMenu.text

            onTriggered: Clipboard.text = messageContextMenu.text
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("Copy &link location")
            visible: messageContextMenu.link

            onTriggered: Clipboard.text = messageContextMenu.link
        }
        Platform.MenuItem {
            id: reactionOption

            text: qsTr("Re&act")
            visible: room ? room.permissions.canSend(MtxEvent.Reaction) : false

            onTriggered: emojiPopup.visible ? emojiPopup.close() : emojiPopup.show(null, room.roomId, function (plaintext, markdown) {
                    room.input.reaction(messageContextMenu.eventId, plaintext);
                    TimelineManager.focusMessageInput();
                })
        }
        Platform.MenuItem {
            text: qsTr("Repl&y")
            visible: room ? room.permissions.canSend(MtxEvent.TextMessage) : false

            onTriggered: room.reply = (messageContextMenu.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Edit")
            visible: messageContextMenu.isEditable && (room ? room.permissions.canSend(MtxEvent.TextMessage) : false)

            onTriggered: room.edit = (messageContextMenu.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Thread")
            visible: (room ? room.permissions.canSend(MtxEvent.TextMessage) : false)

            onTriggered: room.thread = (messageContextMenu.threadId || messageContextMenu.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: visible && room.pinnedMessages.includes(messageContextMenu.eventId) ? qsTr("Un&pin") : qsTr("&Pin")
            visible: (room ? room.permissions.canChange(MtxEvent.PinnedEvents) : false)

            onTriggered: visible && room.pinnedMessages.includes(messageContextMenu.eventId) ? room.unpin(messageContextMenu.eventId) : room.pin(messageContextMenu.eventId)
        }
        Platform.MenuItem {
            text: qsTr("&Read receipts")

            onTriggered: room.showReadReceipts(messageContextMenu.eventId)
        }
        Platform.MenuItem {
            text: qsTr("&Forward")
            visible: messageContextMenu.eventType == MtxEvent.ImageMessage || messageContextMenu.eventType == MtxEvent.VideoMessage || messageContextMenu.eventType == MtxEvent.AudioMessage || messageContextMenu.eventType == MtxEvent.FileMessage || messageContextMenu.eventType == MtxEvent.Sticker || messageContextMenu.eventType == MtxEvent.TextMessage || messageContextMenu.eventType == MtxEvent.LocationMessage || messageContextMenu.eventType == MtxEvent.EmoteMessage || messageContextMenu.eventType == MtxEvent.NoticeMessage

            onTriggered: {
                var forwardMess = forwardCompleterComponent.createObject(timelineRoot);
                forwardMess.setMessageEventId(messageContextMenu.eventId);
                forwardMess.open();
                timelineRoot.destroyOnClose(forwardMess);
            }
        }
        Platform.MenuItem {
            text: qsTr("&Mark as read")
        }
        Platform.MenuItem {
            text: qsTr("View raw message")

            onTriggered: room.viewRawMessage(messageContextMenu.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("View decrypted raw message")
            // TODO(Nico): Fix this still being iterated over, when using keyboard to select options
            visible: messageContextMenu.isEncrypted

            onTriggered: room.viewDecryptedRawMessage(messageContextMenu.eventId)
        }
        Platform.MenuItem {
            text: qsTr("Remo&ve message")
            visible: (room ? room.permissions.canRedact() : false) || messageContextMenu.isSender

            onTriggered: function () {
                var dialog = removeReason.createObject(timelineRoot);
                dialog.eventId = messageContextMenu.eventId;
                dialog.show();
                dialog.forceActiveFocus();
                timelineRoot.destroyOnClose(dialog);
            }
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Save as")
            visible: messageContextMenu.eventType == MtxEvent.ImageMessage || messageContextMenu.eventType == MtxEvent.VideoMessage || messageContextMenu.eventType == MtxEvent.AudioMessage || messageContextMenu.eventType == MtxEvent.FileMessage || messageContextMenu.eventType == MtxEvent.Sticker

            onTriggered: room.saveMedia(messageContextMenu.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Open in external program")
            visible: messageContextMenu.eventType == MtxEvent.ImageMessage || messageContextMenu.eventType == MtxEvent.VideoMessage || messageContextMenu.eventType == MtxEvent.AudioMessage || messageContextMenu.eventType == MtxEvent.FileMessage || messageContextMenu.eventType == MtxEvent.Sticker

            onTriggered: room.openMedia(messageContextMenu.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("Copy link to eve&nt")
            visible: messageContextMenu.eventId

            onTriggered: room.copyLinkToEvent(messageContextMenu.eventId)
        }
    }
    Component {
        id: forwardCompleterComponent

        ForwardCompleter {
        }
    }
    Platform.Menu {
        id: replyContextMenu

        property string eventId
        property string link
        property string text

        function show(text_, link_, eventId_) {
            text = text_;
            link = link_;
            eventId = eventId_;
            open();
        }

        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Copy")
            visible: replyContextMenu.text

            onTriggered: Clipboard.text = replyContextMenu.text
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("Copy &link location")
            visible: replyContextMenu.link

            onTriggered: Clipboard.text = replyContextMenu.link
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Go to quoted message")
            visible: true

            onTriggered: room.showEvent(replyContextMenu.eventId)
        }
    }
    RoundButton {
        id: toEndButton

        property int fullWidth: 40

        flat: true
        height: width
        hoverEnabled: true
        radius: width / 2
        width: 0

        background: Rectangle {
            border.color: toEndButton.hovered ? palette.highlight : palette.buttonText
            border.width: 1
            color: toEndButton.down ? palette.highlight : palette.button
            opacity: enabled ? 1 : 0.3
            radius: toEndButton.radius
        }
        states: [
            State {
                name: ""

                PropertyChanges {
                    target: toEndButton
                    width: 0
                }
            },
            State {
                name: "shown"
                when: !chat.atYEnd

                PropertyChanges {
                    target: toEndButton
                    width: toEndButton.fullWidth
                }
            }
        ]
        transitions: Transition {
            from: ""
            reversible: true
            to: "shown"

            SequentialAnimation {
                PauseAnimation {
                    duration: 500
                }
                PropertyAnimation {
                    duration: 200
                    easing.type: Easing.InOutQuad
                    properties: "width"
                    target: toEndButton
                }
            }
        }

        onClicked: function () {
            chat.positionViewAtBeginning();
            TimelineManager.focusMessageInput();
        }

        anchors {
            bottom: parent.bottom
            bottomMargin: Nheko.paddingMedium + (fullWidth - width) / 2
            right: scrollbar.left
            rightMargin: Nheko.paddingMedium + (fullWidth - width) / 2
        }
        Image {
            id: buttonImg

            anchors.fill: parent
            anchors.margins: Nheko.paddingMedium
            fillMode: Image.PreserveAspectFit
            source: "image://colorimage/:/icons/icons/ui/download.svg?" + (toEndButton.down ? palette.highlightedText : palette.buttonText)
        }
    }
}
