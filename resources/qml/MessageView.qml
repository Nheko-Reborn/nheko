// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./ui"
import "./dialogs"
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
    property bool filterByNotifications: false
    property Room roommodel: room

    // HACK: https://bugreports.qt.io/browse/QTBUG-83972, qtwayland cannot auto hide menu
    Connections {
        function onHideMenu() {
            messageContextMenuC.close();
            replyContextMenuC.close();
        }

        target: MainWindow
    }

    Connections {
        function onScrollToIndex(index) {
            chat.positionViewAtIndex(index, ListView.Center);
            chat.updateLastScroll();
        }

        target: room
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
        model: (filteredTimeline.filterByThread || filteredTimeline.filterByContent || filteredTimeline.filterByNotifications) ? filteredTimeline : room
        //pixelAligned: true
        // 0, not a couple px: any gap here shows through as a seam between the merged bubbles
        // of a same-sender group in TimelineBubbleMessageStyle.qml. Section headers (avatar +
        // name), shown once per group, already provide enough separation between groups.
        spacing: 0
        verticalLayoutDirection: ListView.BottomToTop

        property int lastScrollPos: 0

        // Fixup the scroll position when the height changes. Without this, the view is kept around the center of the currently visible content, while we usually want to stick to the bottom.
        function updateLastScroll() {
            lastScrollPos = (contentY+height);
        }
        onMovementEnded: updateLastScroll()
        // Switching rooms destroys the old delegates. messageActionsC.attached may still be
        // pointing at one of them, which leaves the action popup visibly stuck (wrong position,
        // wrong content, wrong color) since nothing else ever clears that reference.
        onModelChanged: {
            updateLastScroll();
            messageActionsC.attached = null;
            messageActionsC.bubbleMode = false;
            messageActionsC.hoverSource = null;
        }
        onHeightChanged: contentY = (lastScrollPos-height)

        Component {
            id: defaultMessageStyle

            TimelineDefaultMessageStyle {
                messageActions: messageActionsC
                messageContextMenu: messageContextMenuC
                replyContextMenu: replyContextMenuC
                scrolledToThis: eventId === room.scrollTarget && (y + height > chat.y + chat.contentY && y < chat.y + chat.height + chat.contentY)
                data: [
                    Connections {
                        function onMovementEnded() {
                            if (y + height + 2 * chat.spacing > chat.contentY + chat.height && y < chat.contentY + chat.height) {
                                room.currentIndex = index;
                            }
                        }
                        target: chat
                    }
                ]
            }
        }
        Component {
            id: bubbleMessageStyle

            TimelineBubbleMessageStyle {
                messageActions: messageActionsC
                messageContextMenu: messageContextMenuC
                replyContextMenu: replyContextMenuC
                scrolledToThis: eventId === room.scrollTarget && (y + height > chat.y + chat.contentY && y < chat.y + chat.height + chat.contentY)
                data: [
                    Connections {
                        function onMovementEnded() {
                            if (y + height + 2 * chat.spacing > chat.contentY + chat.height && y < chat.contentY + chat.height) {
                                room.currentIndex = index;
                            }
                        }
                        target: chat
                    }
                ]
            }
        }

        delegate: Settings.bubbles ? bubbleMessageStyle : defaultMessageStyle
        footer: Item {
            width: chat.delegateMaxWidth
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
            filterByNotifications: chatRoot.filterByNotifications
            filterByThread: room ? room.thread : ""
            source: room
        }
        Control {
            id: messageActionsC

            property Item attached: null
            // use comma to update on scroll
            property alias model: row.model
            // True for bubble style's positioning (inline with whichever specific message is
            // hovered, just past its text); false for the legacy default style (float above,
            // right edge near the metadata).
            property bool bubbleMode: false
            // Only meaningful when bubbleMode: true for our own (right-aligned) messages, whose
            // popup mirrors to sit left of their text instead of right of it - see
            // TimelineBubbleMessageStyle's textStartOffset/anchorFromRight.
            property bool anchorFromRight: false
            // Set by whichever message's own HoverHandler is currently active. Each message in a
            // merged bubble is its own delegate with its own popup position, but the pointer can
            // linger on the popup itself between two such positions, so visibility can't rely on
            // attached.hovered alone.
            property bool sourceHovered: false
            // The specific delegate whose HoverHandler last set sourceHovered true. Moving the
            // pointer directly from one message to a neighboring one can deliver the old
            // message's "leave" after the new message's "enter" (which already moved attached
            // elsewhere) - comparing against attached itself wouldn't catch that reordering,
            // since by the time the stale leave arrives attached already points elsewhere; this
            // tracks the exact delegate so only its own leave can clear the hover it set.
            property Item hoverSource: null

            // Raw preferred offsets, set imperatively by whichever style/message is hovered. Kept
            // separate from the anchor margins themselves (computed below as bindings) because a
            // Qt anchor margin, once ever assigned imperatively, permanently stops being a live
            // binding - and this single Control is shared between bubble and default style, with
            // bubbleMode able to flip at runtime if the user toggles the bubbles setting, so every
            // margin here has to stay a pure binding driven by these plain (imperative-write-safe)
            // properties instead of ever being assigned directly.
            property real preferredLeftOffset: 0
            // bubbleMode + anchorFromRight (our own messages): offset from attached's own left to
            // where the popup's RIGHT edge should land (just before the text start). Kept in
            // "distance from the left" terms, like preferredLeftOffset, rather than switching to
            // an actual anchors.right binding: toggling a Qt Quick anchor between two different
            // target lines (left vs. right) repeatedly at runtime - which bubbleMode does on every
            // hover, since anchorFromRight flips per message - doesn't reliably detach the old
            // line, and can leave both edges anchored at once, stretching the popup to fill the
            // entire attached width. Keeping bubbleMode on a single, never-swapped anchors.left
            // avoids that; only default style (which never flips) uses anchors.right.
            property real preferredRightEdgeOffset: 0
            // Default style only: plain right-edge offset (metadata.width), consumed via
            // anchors.rightMargin - safe there since default style's anchor side never changes.
            property real preferredRightOffset: 0
            // bubbleMode: offset from attached's own top to the vertical center of that specific
            // message's own bubble - see TimelineBubbleMessageStyle's bubbleCenterOffset.
            property real preferredTopOffset: 0
            // Default style: offset from attached's own top to where the popup's bottom edge
            // should sit (equivalent to the old "-gridContainer.y" expression).
            property real preferredBottomOffset: 0

            hoverEnabled: true
            padding: Nheko.paddingSmall + 2
            // Moving the pointer from the message to the popup (or between messages) passes
            // through a moment where neither sourceHovered nor this control's own hovered is
            // true yet - hiding immediately on that means the pointer arrives at an
            // already-invisible item, which never gets a fresh enter event, so it stays hidden.
            // hideTimer bridges that gap.
            property bool shouldBeVisible: Settings.buttonsInTimeline && !!attached && (sourceHovered || hovered)
            visible: shouldBeVisible || hideTimer.running
            z: 10
            // Stays parented at the top level, a sibling of every ListView delegate (attached),
            // rather than reparented onto the bubble: that seemed simpler, since it makes the
            // bubble's own edges valid anchor targets, but its mapped screen position came out
            // wrong (probably an interaction with the ListView's BottomToTop layout and
            // reparenting mid-layout) - the popup rendered in one place but hit-tested in
            // another, which looks exactly like "vanishes when the pointer reaches it". Anchoring
            // to attached (a valid sibling) with margins computed from the bubble's geometry,
            // the same approach already proven for the beside-the-bubble placement, avoids it.
            parent: chat.contentItem
            anchors.top: bubbleMode ? attached?.top : undefined
            anchors.topMargin: bubbleMode ? (preferredTopOffset - height / 2) : 0
            anchors.bottom: bubbleMode ? undefined : attached?.top
            anchors.bottomMargin: bubbleMode ? 0 : preferredBottomOffset
            // Bubble style always anchors by its left edge (see preferredRightEdgeOffset above for
            // why anchorFromRight doesn't switch to anchors.right instead) - just past the hovered
            // message's own text, or for our own messages, offset back from the desired right edge
            // by this popup's own width so that edge lands just before the text start. A short
            // message in a group of otherwise-long ones would otherwise leave the popup stranded
            // far from its own text if it stayed anchored to the group's shared bubble edge.
            // Default style keeps the original right-edge anchoring.
            anchors.left: bubbleMode ? attached?.left : undefined
            anchors.leftMargin: bubbleMode ? (anchorFromRight ? Math.max(0, Math.min(preferredRightEdgeOffset - width, (attached?.width ?? 0) - width)) : Math.max(0, Math.min(preferredLeftOffset, (attached?.width ?? 0) - width))) : 0
            anchors.right: bubbleMode ? undefined : attached?.right
            anchors.rightMargin: bubbleMode ? 0 : preferredRightOffset

            onShouldBeVisibleChanged: {
                if (shouldBeVisible)
                    hideTimer.stop();
                else
                    hideTimer.restart();
            }

            Timer {
                id: hideTimer
                interval: 300
            }

            background: Rectangle {
                border.color: palette.buttonText
                border.width: 1
                // Match the bubble it's attached to, so it reads as part of the same message
                // instead of a generic floating toolbar.
                color: (messageActionsC.bubbleMode && messageActionsC.attached?.bubble) ? messageActionsC.attached.bubble.bubbleColor : palette.window
                radius: padding
            }
            contentItem: RowLayout {
                id: row

                property var model

                spacing: messageActionsC.padding

                // Exactly three actions, matching Signal's hover toolbar: react (a generic,
                // uncolored heart - opens the emoji picker rather than sending a fixed emoji
                // itself, hence the "+" badge), reply, and a catch-all options menu for
                // everything else (edit, threads, pin, forward, go to message, ...).
                ImageButton {
                    id: reactButton

                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("React")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/heart-add.svg"
                    visible: room ? room.permissions.canSend(MtxEvent.Reaction) : false
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20

                    onClicked: emojiPopup.visible ? emojiPopup.close() : emojiPopup.show(reactButton, room.roomId, function (plaintext, markdown) {
                            var event_id = row.model ? row.model.eventId : "";
                            room.input.reaction(event_id, plaintext);
                            TimelineManager.focusMessageInput();
                        })
                }
                ImageButton {
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Reply")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/reply.svg"
                    visible: room ? room.permissions.canSend(MtxEvent.TextMessage) : false
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20

                    onClicked: room.reply = row.model.eventId
                }
                ImageButton {
                    id: optionsButton

                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Options")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/options.svg"
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20

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
            // We don't want this to steal the focus from other dialogs!
            // Workaround for https://qt-project.atlassian.net/browse/QTBUG-141691
            id: s
            enabled: Nheko.focusWindow == Nheko.findWindow(s)

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
    Menu {
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

            messageActionsCFilter.updateTarget();

            if (showAt_)
                popup(showAt_);
            else
                popup();
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

        Component.onCompleted: {
            if (messageContextMenuC.popupType != undefined) {
                messageContextMenuC.popupType = 2; // Popup.Native with fallback on older Qt (<6.8.0)
            }
        }

        NhekoMenuVisibilityFilter on contentData {
            id: messageActionsCFilter

            Component {
                MenuItem {
                    text: qsTr("Go to &message")
                    visible: filteredTimeline.filterByContent

                    onTriggered: function () {
                        topBar.searchString = "";
                        room.showEvent(messageContextMenuC.eventId);
                    }
                }
            }
            Component {
                MenuItem {
                    text: qsTr("&Copy")
                    visible: messageContextMenuC.text

                    onTriggered: Clipboard.text = messageContextMenuC.text
                }
            }
            Component {
                MenuItem {
                    text: qsTr("Copy &link location")
                    visible: messageContextMenuC.link

                    onTriggered: Clipboard.text = messageContextMenuC.link
                }
            }
            Component {
                MenuItem {
                    id: reactionOption

                    text: qsTr("Re&act")
                    visible: room ? room.permissions.canSend(MtxEvent.Reaction) : false

                    onTriggered: emojiPopup.visible ? emojiPopup.close() : emojiPopup.show(null, room.roomId, function (plaintext, markdown) {
                        room.input.reaction(messageContextMenuC.eventId, plaintext);
                        TimelineManager.focusMessageInput();
                    })
                }
            }
            Component {
                MenuItem {
                    text: qsTr("Repl&y")
                    visible: room ? room.permissions.canSend(MtxEvent.TextMessage) : false

                    onTriggered: room.reply = (messageContextMenuC.eventId)
                }
            }
            Component {
                MenuItem {
                    text: qsTr("&Edit")
                    visible: messageContextMenuC.isEditable && (room ? room.permissions.canSend(MtxEvent.TextMessage) : false)

                    onTriggered: room.edit = (messageContextMenuC.eventId)
                }
            }
            Component {
                MenuItem {
                    text: qsTr("&Thread")
                    visible: (room ? room.permissions.canSend(MtxEvent.TextMessage) : false)

                    onTriggered: room.thread = (messageContextMenuC.threadId || messageContextMenuC.eventId)
                }
            }
            Component {
                MenuItem {
                    text: visible && room.pinnedMessages.includes(messageContextMenuC.eventId) ? qsTr("Un&pin") : qsTr("&Pin")
                    visible: (room ? room.permissions.canChange(MtxEvent.PinnedEvents) : false)

                    onTriggered: visible && room.pinnedMessages.includes(messageContextMenuC.eventId) ? room.unpin(messageContextMenuC.eventId) : room.pin(messageContextMenuC.eventId)
                }
            }
            Component {
                MenuItem {
                    text: qsTr("&Read receipts")

                    onTriggered: room.showReadReceipts(messageContextMenuC.eventId)
                }
            }
            Component {
                MenuItem {
                    text: qsTr("&Forward")
                    visible: messageContextMenuC.eventType == MtxEvent.ImageMessage || messageContextMenuC.eventType == MtxEvent.VideoMessage || messageContextMenuC.eventType == MtxEvent.AudioMessage || messageContextMenuC.eventType == MtxEvent.FileMessage || messageContextMenuC.eventType == MtxEvent.Sticker || messageContextMenuC.eventType == MtxEvent.TextMessage || messageContextMenuC.eventType == MtxEvent.LocationMessage || messageContextMenuC.eventType == MtxEvent.EmoteMessage || messageContextMenuC.eventType == MtxEvent.NoticeMessage

                    onTriggered: {
                        var forwardMess = forwardCompleterComponent.createObject(timelineRoot);
                        forwardMess.setMessageEventId(messageContextMenuC.eventId);
                        forwardMess.open();
                        timelineRoot.destroyOnClose(forwardMess);
                    }
                }
            }
            Component {
                MenuItem {
                    text: qsTr("&Mark as read")

                    onTriggered: room.markEventAsRead(messageContextMenuC.eventId)
                }
            }
            Component {
                MenuItem {
                    text: qsTr("View raw message")

                    onTriggered: room.viewRawMessage(messageContextMenuC.eventId)
                }
            }
            Component {
                MenuItem {
                    text: qsTr("View decrypted raw message")
                    // TODO(Nico): Fix this still being iterated over, when using keyboard to select options
                    visible: messageContextMenuC.isEncrypted

                    onTriggered: room.viewDecryptedRawMessage(messageContextMenuC.eventId)
                }
            }
            Component {
                MenuItem {
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
            }
            Component {
                MenuItem {
                    text: qsTr("Report message")
                    onTriggered: function () {
                        var dialog = reportDialog.createObject(timelineRoot, {"eventId": messageContextMenuC.eventId});
                        dialog.show();
                        dialog.forceActiveFocus();
                        timelineRoot.destroyOnClose(dialog);
                    }
                }
            }
            Component {
                MenuItem {
                    text: qsTr("&Save as")
                    visible: messageContextMenuC.eventType == MtxEvent.ImageMessage || messageContextMenuC.eventType == MtxEvent.VideoMessage || messageContextMenuC.eventType == MtxEvent.AudioMessage || messageContextMenuC.eventType == MtxEvent.FileMessage || messageContextMenuC.eventType == MtxEvent.Sticker

                    onTriggered: room.saveMedia(messageContextMenuC.eventId)
                }
            }
            Component {
                MenuItem {
                    text: qsTr("&Open in external program")
                    visible: messageContextMenuC.eventType == MtxEvent.ImageMessage || messageContextMenuC.eventType == MtxEvent.VideoMessage || messageContextMenuC.eventType == MtxEvent.AudioMessage || messageContextMenuC.eventType == MtxEvent.FileMessage || messageContextMenuC.eventType == MtxEvent.Sticker

                    onTriggered: room.openMedia(messageContextMenuC.eventId)
                }
            }
            Component {
                MenuItem {
                    text: qsTr("Copy link to eve&nt")
                    visible: messageContextMenuC.eventId

                    onTriggered: room.copyLinkToEvent(messageContextMenuC.eventId)
                }
            }
        }
    }
    Component {
        id: forwardCompleterComponent

        ForwardCompleter {
        }
    }
    Menu {
        id: replyContextMenuC

        property string eventId
        property string link
        property string text

        function show(text_, link_, eventId_) {
            text = text_;
            link = link_;
            eventId = eventId_;

            replyContextMenuCFilter.updateTarget();
            popup();
        }

        Component.onCompleted: {
            if (replyContextMenuC.popupType != undefined) {
                replyContextMenuC.popupType = 2; // Popup.Native with fallback on older Qt (<6.8.0)
            }
        }


        NhekoMenuVisibilityFilter on contentData {
            id: replyContextMenuCFilter

            Component {
                MenuItem {
                    text: qsTr("&Copy")
                    visible: replyContextMenuC.text

                    onTriggered: Clipboard.text = replyContextMenuC.text
                }
            }
            Component {
                MenuItem {
                    text: qsTr("Copy &link location")
                    visible: replyContextMenuC.link

                    onTriggered: Clipboard.text = replyContextMenuC.link
                }
            }
            Component {
                MenuItem {
                    text: qsTr("&Go to quoted message")
                    visible: true

                    onTriggered: room.showEvent(replyContextMenuC.eventId)
                }
            }
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
            chat.updateLastScroll();
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
