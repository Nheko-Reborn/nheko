// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    property Room roommodel: room

    // HACK: https://bugreports.qt.io/browse/QTBUG-83972, qtwayland cannot auto hide menu
    Connections {
        function onHideMenu() {
            messageContextMenuC.close();
            replyContextMenuC.close();
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
        displayMarginBeginning: height / 4
        displayMarginEnd: height / 4
        model: (filteredTimeline.filterByThread || filteredTimeline.filterByContent) ? filteredTimeline : room
        //pixelAligned: true
        spacing: 2
        verticalLayoutDirection: ListView.BottomToTop

        property int lastScrollPos: 0

        // Fixup the scroll position when the height changes. Without this, the view is kept around the center of the currently visible content, while we usually want to stick to the bottom.
        onMovementEnded: lastScrollPos = (contentY+height)
        onModelChanged: lastScrollPos = (contentY+height)
        onHeightChanged: contentY = (lastScrollPos-height)

        Component {
            id: defaultMessageStyle

            TimelineDefaultMessageStyle {
                messageActions: messageActionsC
                messageContextMenu: messageContextMenuC
                replyContextMenu: replyContextMenuC
                scrolledToThis: eventId === room.scrollTarget && (y + height > chat.y + chat.contentY && y < chat.y + chat.height + chat.contentY)
            }
        }
        Component {
            id: bubbleMessageStyle

            TimelineBubbleMessageStyle {
                messageActions: messageActionsC
                messageContextMenu: messageContextMenuC
                replyContextMenu: replyContextMenuC
                scrolledToThis: eventId === room.scrollTarget && (y + height > chat.y + chat.contentY && y < chat.y + chat.height + chat.contentY)
            }
        }

        delegate: Settings.bubbles ? bubbleMessageStyle : defaultMessageStyle
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
            id: messageActionsC

            property Item attached: null
            // use comma to update on scroll
            property alias model: row.model

            hoverEnabled: true
            padding: Nheko.paddingSmall
            visible: Settings.buttonsInTimeline && !!attached && (attached.hovered || hovered)
            z: 10
            parent: chat.contentItem
            anchors.bottom: attached?.top
            anchors.right: attached?.right

            background: Rectangle {
                border.color: palette.buttonText
                border.width: 1
                color: palette.window
                radius: padding
            }
            contentItem: RowLayout {
                id: row

                property var model

                spacing: messageActionsC.padding

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
                            // Workaround, can't get icon.source working for now...
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            source: button.showImage ? (button.modelData.replace("mxc://", "image://MxcImage/") + "?scale") : ""
                            sourceSize.height: button.height
                            sourceSize.width: button.width
                        }
                        NhekoCursorShape {
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
                    Layout.preferredWidth: 16

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
                    Layout.preferredWidth: 16

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
                    Layout.preferredWidth: 16

                    onClicked: room.thread = (row.model.threadId || row.model.eventId)
                }
                ImageButton {
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Reply")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/reply.svg"
                    visible: room ? room.permissions.canSend(MtxEvent.TextMessage) : false
                    Layout.preferredWidth: 16

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
                    Layout.preferredWidth: 16

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
                    Layout.preferredWidth: 16

                    onClicked: messageContextMenuC.show(row.model.eventId, row.model.threadId, row.model.type, row.model.isSender, row.model.isEncrypted, row.model.isEditable, "", row.model.body, optionsButton)
                }
            }
        }
        Shortcut {
            sequences: [StandardKey.MoveToPreviousPage]

            onActivated: {
                chat.contentY = chat.contentY - chat.height * 0.9;
                chat.returnToBounds();
            }
        }
        Shortcut {
            sequences: [StandardKey.MoveToNextPage]

            onActivated: {
                chat.contentY = chat.contentY + chat.height * 0.9;
                chat.returnToBounds();
            }
        }
        Shortcut {
            sequences: [StandardKey.Cancel]

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
    }
    Platform.Menu {
        id: messageContextMenuC

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
        Component {
            id: reportDialog

            ReportMessage {}
        }

        Platform.MenuItem {
            enabled: visible
            text: qsTr("Go to &message")
            visible: filteredTimeline.filterByContent

            onTriggered: function () {
                topBar.searchString = "";
                room.showEvent(messageContextMenuC.eventId);
            }
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Copy")
            visible: messageContextMenuC.text

            onTriggered: Clipboard.text = messageContextMenuC.text
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("Copy &link location")
            visible: messageContextMenuC.link

            onTriggered: Clipboard.text = messageContextMenuC.link
        }
        Platform.MenuItem {
            id: reactionOption

            text: qsTr("Re&act")
            visible: room ? room.permissions.canSend(MtxEvent.Reaction) : false

            onTriggered: emojiPopup.visible ? emojiPopup.close() : emojiPopup.show(null, room.roomId, function (plaintext, markdown) {
                    room.input.reaction(messageContextMenuC.eventId, plaintext);
                    TimelineManager.focusMessageInput();
                })
        }
        Platform.MenuItem {
            text: qsTr("Repl&y")
            visible: room ? room.permissions.canSend(MtxEvent.TextMessage) : false

            onTriggered: room.reply = (messageContextMenuC.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Edit")
            visible: messageContextMenuC.isEditable && (room ? room.permissions.canSend(MtxEvent.TextMessage) : false)

            onTriggered: room.edit = (messageContextMenuC.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Thread")
            visible: (room ? room.permissions.canSend(MtxEvent.TextMessage) : false)

            onTriggered: room.thread = (messageContextMenuC.threadId || messageContextMenuC.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: visible && room.pinnedMessages.includes(messageContextMenuC.eventId) ? qsTr("Un&pin") : qsTr("&Pin")
            visible: (room ? room.permissions.canChange(MtxEvent.PinnedEvents) : false)

            onTriggered: visible && room.pinnedMessages.includes(messageContextMenuC.eventId) ? room.unpin(messageContextMenuC.eventId) : room.pin(messageContextMenuC.eventId)
        }
        Platform.MenuItem {
            text: qsTr("&Read receipts")

            onTriggered: room.showReadReceipts(messageContextMenuC.eventId)
        }
        Platform.MenuItem {
            text: qsTr("&Forward")
            visible: messageContextMenuC.eventType == MtxEvent.ImageMessage || messageContextMenuC.eventType == MtxEvent.VideoMessage || messageContextMenuC.eventType == MtxEvent.AudioMessage || messageContextMenuC.eventType == MtxEvent.FileMessage || messageContextMenuC.eventType == MtxEvent.Sticker || messageContextMenuC.eventType == MtxEvent.TextMessage || messageContextMenuC.eventType == MtxEvent.LocationMessage || messageContextMenuC.eventType == MtxEvent.EmoteMessage || messageContextMenuC.eventType == MtxEvent.NoticeMessage

            onTriggered: {
                var forwardMess = forwardCompleterComponent.createObject(timelineRoot);
                forwardMess.setMessageEventId(messageContextMenuC.eventId);
                forwardMess.open();
                timelineRoot.destroyOnClose(forwardMess);
            }
        }
        Platform.MenuItem {
            text: qsTr("&Mark as read")
        }
        Platform.MenuItem {
            text: qsTr("View raw message")

            onTriggered: room.viewRawMessage(messageContextMenuC.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("View decrypted raw message")
            // TODO(Nico): Fix this still being iterated over, when using keyboard to select options
            visible: messageContextMenuC.isEncrypted

            onTriggered: room.viewDecryptedRawMessage(messageContextMenuC.eventId)
        }
        Platform.MenuItem {
            text: qsTr("Remo&ve message")
            visible: (room ? room.permissions.canRedact() : false) || messageContextMenuC.isSender

            onTriggered: function () {
                var dialog = removeReason.createObject(timelineRoot);
                dialog.eventId = messageContextMenuC.eventId;
                dialog.show();
                dialog.forceActiveFocus();
                timelineRoot.destroyOnClose(dialog);
            }
        }
        Platform.MenuItem {
            text: qsTr("Report message")
            enabled: visible
            onTriggered: function () {
                var dialog = reportDialog.createObject(timelineRoot, {"eventId": messageContextMenuC.eventId});
                dialog.show();
                dialog.forceActiveFocus();
                timelineRoot.destroyOnClose(dialog);
            }
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Save as")
            visible: messageContextMenuC.eventType == MtxEvent.ImageMessage || messageContextMenuC.eventType == MtxEvent.VideoMessage || messageContextMenuC.eventType == MtxEvent.AudioMessage || messageContextMenuC.eventType == MtxEvent.FileMessage || messageContextMenuC.eventType == MtxEvent.Sticker

            onTriggered: room.saveMedia(messageContextMenuC.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Open in external program")
            visible: messageContextMenuC.eventType == MtxEvent.ImageMessage || messageContextMenuC.eventType == MtxEvent.VideoMessage || messageContextMenuC.eventType == MtxEvent.AudioMessage || messageContextMenuC.eventType == MtxEvent.FileMessage || messageContextMenuC.eventType == MtxEvent.Sticker

            onTriggered: room.openMedia(messageContextMenuC.eventId)
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("Copy link to eve&nt")
            visible: messageContextMenuC.eventId

            onTriggered: room.copyLinkToEvent(messageContextMenuC.eventId)
        }
    }
    Component {
        id: forwardCompleterComponent

        ForwardCompleter {
        }
    }
    Platform.Menu {
        id: replyContextMenuC

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
            visible: replyContextMenuC.text

            onTriggered: Clipboard.text = replyContextMenuC.text
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("Copy &link location")
            visible: replyContextMenuC.link

            onTriggered: Clipboard.text = replyContextMenuC.link
        }
        Platform.MenuItem {
            enabled: visible
            text: qsTr("&Go to quoted message")
            visible: true

            onTriggered: room.showEvent(replyContextMenuC.eventId)
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
                    toEndButton.width: 0
                }
            },
            State {
                name: "shown"
                when: !chat.atYEnd

                PropertyChanges {
                    toEndButton.width: toEndButton.fullWidth
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
            anchors.fill: parent
            anchors.margins: Nheko.paddingMedium
            fillMode: Image.PreserveAspectFit
            source: "image://colorimage/:/icons/icons/ui/download.svg?" + (toEndButton.down ? palette.highlightedText : palette.buttonText)
        }
    }
}
