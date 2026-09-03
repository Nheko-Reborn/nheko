// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import im.nheko

TimelineEvent {
    id: wrapper
    ListView.delayRemove: true
    width: chat.delegateMaxWidth
    // We return a larger size for any item but the most bottom one, if it isn't initialized yet, since otherwise Qt will create way too many items.
    // If we did that also for the first item, it would mess with the scroll location a bit, so we don't do it for that item.
    height: Math.max((section.item?.height ?? 0) + ((gridContainer.implicitHeight < 1 && index != 0) ? 100 : gridContainer.implicitHeight) + reactionRow.implicitHeight + unreadRow.height, 10)
    anchors.horizontalCenter: ListView.view.contentItem.horizontalCenter
    //room: chatRoot.roommodel

    required property var day
    required property bool isSender
    required property int index
    property var previousMessageDay: (index + 1) >= chat.count ? 0 : chat.model.dataByIndex(index + 1, Room.Day)
    property var previousMessageTimestamp: (index + 1) >= chat.count ? 0 : chat.model.dataByIndex(index + 1, Room.Timestamp)
    property bool previousMessageIsStateEvent: (index + 1) >= chat.count ? true : chat.model.dataByIndex(index + 1, Room.IsStateEvent)
    property string previousMessageUserId: (index + 1) >= chat.count ? "" : chat.model.dataByIndex(index + 1, Room.UserId)

    // The newer neighbor (index - 1), used to tell whether this message is the last one in a
    // consecutive run from the same sender, so its bubble can be merged visually with the next.
    property var nextMessageDay: (index - 1) < 0 ? 0 : chat.model.dataByIndex(index - 1, Room.Day)
    property var nextMessageTimestamp: (index - 1) < 0 ? 0 : chat.model.dataByIndex(index - 1, Room.Timestamp)
    property bool nextMessageIsStateEvent: (index - 1) < 0 ? true : chat.model.dataByIndex(index - 1, Room.IsStateEvent)
    property string nextMessageUserId: (index - 1) < 0 ? "" : chat.model.dataByIndex(index - 1, Room.UserId)

    required property date timestamp
    required property string userId
    required property string userName
    required property string threadId
    required property int userPowerlevel
    required property bool isEdited
    required property bool isEncrypted
    required property var reactions
    required property int status
    required property int trustlevel
    required property int notificationlevel
    required property int type
    required property bool isEditable

    required property QtObject messageContextMenu
    required property QtObject replyContextMenu
    required property Item messageActions

    property int avatarMargin: (wrapper.isStateEvent || Settings.smallAvatars ? 0 : (Nheko.avatarSize + 8)) // align bubble with section header

    // Exposed so messageActions can be reparented directly onto the bubble while hovering it -
    // anchoring straight to a direct parent avoids fragile cross-item coordinate math entirely.
    property alias bubble: messageBubble

    property alias hovered: messageHover.hovered

    // Raw content width of just this message, exposed so grouped neighbors can read it without
    // recursing into each other's already-matched (see groupWidth) width.
    property alias groupItemWidth: contentPlacementContainer.ownContentWidth

    // All bubbles in a consecutive-sender group share one width - the widest message in it -
    // so the group reads as a single block with straight sides instead of a jagged stack where
    // each bubble is only as wide as its own text.
    property real groupWidth: {
        let w = wrapper.groupItemWidth
        let view = wrapper.ListView.view
        if (view) {
            if (!wrapper.isGroupStart) {
                let i = wrapper.index + 1
                while (i < chat.count) {
                    let item = view.itemAtIndex(i)
                    if (!item)
                        break
                    w = Math.max(w, item.groupItemWidth)
                    if (item.isGroupStart)
                        break
                    i++
                }
            }
            if (!wrapper.isGroupEnd) {
                let i = wrapper.index - 1
                while (i >= 0) {
                    let item = view.itemAtIndex(i)
                    if (!item)
                        break
                    w = Math.max(w, item.groupItemWidth)
                    if (item.isGroupEnd)
                        break
                    i--
                }
            }
        }
        return w
    }

    property int oneHour: 60 * 60 * 1000
    property bool showSection: wrapper.previousMessageDay !== wrapper.day || wrapper.timestamp - wrapper.previousMessageTimestamp > oneHour

    // Whether this message is the first/last of a consecutive, same-sender, same-day run within
    // an hour of its neighbor - used to merge their bubbles into one continuous shape.
    property bool isGroupStart: wrapper.previousMessageUserId !== wrapper.userId || wrapper.showSection || wrapper.previousMessageIsStateEvent !== wrapper.isStateEvent
    property bool isGroupEnd: wrapper.nextMessageUserId !== wrapper.userId || wrapper.nextMessageDay !== wrapper.day || wrapper.nextMessageTimestamp - wrapper.timestamp > oneHour || wrapper.nextMessageIsStateEvent !== wrapper.isStateEvent

    // This message's own bubble, vertically centered, as an offset from this wrapper's own top -
    // each message in a merged group is still its own delegate/bubble internally (only their
    // touching edges are squared off to look continuous), so this is specific to the exact
    // message being hovered, not the group as a whole.
    property real bubbleCenterOffset: gridContainer.y + messageBubble.y + messageBubble.height / 2

    // Where this specific message's own text ends/starts, as an offset from this wrapper's own
    // left. groupItemWidth is this message's natural (pre-group-stretch) content width, so a
    // short message in a group that's mostly long messages still gets an offset landing just
    // past (or before) its own short line, not the group's shared (stretched) bubble edge.
    // Text is left-aligned within the bubble for other senders (see contentColumn below), so
    // textEndOffset is what matters there; it's right-aligned for our own messages (mirrored,
    // to leave slack space on the left instead of the right), so textStartOffset matters there.
    property real textEndOffset: gridContainer.x + messageBubble.x + messageBubble.leftPadding + wrapper.groupItemWidth
    property real textStartOffset: gridContainer.x + messageBubble.x + messageBubble.width - messageBubble.rightPadding - wrapper.groupItemWidth

    mainInset: (threadId ? (4 + Nheko.paddingSmall) : 0) + 4
    replyInset: mainInset + 4 + Nheko.paddingSmall

    property int bubbleMargin: 40

    maxWidth: chat.delegateMaxWidth - avatarMargin - bubbleMargin

    data: [
        Loader {
            id: section

            active: wrapper.isGroupStart
            //asynchronous: true
            sourceComponent: TimelineSectionHeader {
                day: wrapper.day
                isSender: wrapper.isSender
                isStateEvent: wrapper.isStateEvent
                parentWidth: wrapper.width
                previousMessageDay: wrapper.previousMessageDay
                previousMessageTimestamp: wrapper.previousMessageTimestamp
                previousMessageIsStateEvent: wrapper.previousMessageIsStateEvent
                previousMessageUserId: wrapper.previousMessageUserId
                timestamp: wrapper.timestamp
                userId: wrapper.userId
                userName: wrapper.userName
                userPowerlevel: wrapper.userPowerlevel
            }
            visible: status == Loader.Ready
            z: 4
        }, 
        Rectangle {
            anchors.fill: gridContainer
            property color threadColor: TimelineManager.userColor(wrapper.threadId, palette.base)
            property color threadBackgroundColor: wrapper.threadId ? Qt.tint(palette.base, Qt.hsla(threadColor.hslHue, 0.7, threadColor.hslLightness, 0.1)) : "transparent"
            color: (Settings.messageHoverHighlight && messageHover.hovered) ? palette.alternateBase : threadBackgroundColor

            // this looks better without margins
            TapHandler {
                acceptedButtons: Qt.RightButton
                acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                gesturePolicy: TapHandler.ReleaseWithinBounds

                onSingleTapped: messageContextMenu.show(wrapper.eventId, wrapper.threadId, wrapper.type, wrapper.isSender, wrapper.isEncrypted, wrapper.isEditable, wrapper.main.hoveredLink, wrapper.main.copyText)
            }
        },
        Rectangle {
            id: scrollHighlight
            anchors.fill: gridContainer

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
                        script: wrapper.room.eventShown()
                    }
                }
            }
        },
        Item {
            id: gridContainer

            width: wrapper.width - wrapper.avatarMargin
            implicitHeight: messageBubble.implicitHeight
            x: wrapper.avatarMargin
            y: section.visible && section.active ? section.y + section.height : 0

            HoverHandler {
                id: messageHover
                blocking: false
                onHoveredChanged: () => {
                    if (Settings.mobileMode)
                        return;
                    if (hovered) {
                        if (!messageActions.hovered) {
                            // Every message in a merged bubble is still its own delegate/bubble
                            // internally, so each one positions its own popup right beside its
                            // own text - inline with that exact message's row, not the group as
                            // a whole. Hovering a short message next to a long one in the same
                            // group therefore shows the popup right next to the short text, not
                            // stranded at the group's shared (stretched) edge.
                            messageActions.model = wrapper;
                            messageActions.bubbleMode = true;
                            // Own messages (isSender) mirror the layout: text is right-aligned
                            // within the bubble (see contentColumn below) so the popup goes to
                            // its left, using the freed-up space on that side instead of the
                            // right - matching Signal's own-message-on-the-right convention.
                            messageActions.anchorFromRight = wrapper.isSender;
                            // Set before attached, which makes messageActions.visible true
                            // immediately - see the equivalent comment in
                            // TimelineDefaultMessageStyle.qml.
                            messageActions.preferredTopOffset = wrapper.bubbleCenterOffset;
                            if (wrapper.isSender)
                                messageActions.preferredRightEdgeOffset = wrapper.textStartOffset - Nheko.paddingSmall;
                            else
                                messageActions.preferredLeftOffset = wrapper.textEndOffset + Nheko.paddingSmall;
                            messageActions.attached = wrapper;
                        }
                        messageActions.hoverSource = wrapper;
                        messageActions.sourceHovered = true;
                    } else if (messageActions.hoverSource === wrapper) {
                        // Only clear if this exact message is still the one that last set
                        // sourceHovered. Moving the pointer directly from this message to a
                        // neighboring one can deliver this "leave" after the neighbor's "enter" -
                        // which has already redirected attached/hoverSource there - and that
                        // stale leave must not clobber the newer, still-active hover.
                        messageActions.sourceHovered = false;
                    }
                }

            }


            AbstractButton {
                id: messageBubble

                anchors.left: (wrapper.isStateEvent || wrapper.isSender) ? undefined : parent.left // qmllint disable Quick.anchor-combinations
                anchors.right: (wrapper.isStateEvent || !wrapper.isSender) ? undefined : parent.right
                anchors.horizontalCenter: wrapper.isStateEvent ? parent.horizontalCenter : undefined

                // One color for your own messages, one flat neutral for everyone else's - a
                // distinct hue-tinted bubble per sender adds up to a lot of clashing colors on
                // screen in an active room.

                contentItem: Item {
                    id: contentPlacementContainer

                    property bool fitsMetadata: ((wrapper.main?.width ?? 0) + wrapper.mainInset + metadata.width) < wrapper.maxWidth

                    // This doesnt work because of tables. They might have content in the top of the cell, while the background reaches to the bottom. Maybe using the textDocument we could do more?
                    // property bool fitsMetadataInside: wrapper.main?.positionAt ? (wrapper.main.positionAt(wrapper.main.width, wrapper.main.height - 4) == wrapper.main.positionAt(wrapper.main.width - metadata.width, wrapper.main.height - 4)) : false
                    property bool fitsMetadataInside: false

                    property real ownContentWidth: Math.max((wrapper.reply?.width ?? 0) + wrapper.replyInset, (wrapper.main?.width ?? 0) + wrapper.mainInset + ((fitsMetadata && !fitsMetadataInside) ? metadata.width : 0))

                    // Every bubble in a group is matched to the group's widest message (see
                    // wrapper.groupWidth) rather than just its own, so the group has straight
                    // sides instead of a jagged outline.
                    implicitWidth: wrapper.isStateEvent ? ownContentWidth : wrapper.groupWidth
                    implicitHeight: contentColumn.implicitHeight + ((fitsMetadata || fitsMetadataInside) ? 0 : metadata.height)

                    TimelineMetadata {
                        id: metadata

                        scaling: 0.75

                        anchors.right: parent.right
                        anchors.bottom: parent.bottom

                        // Same reasoning as the default style: repeating this on every bubble is
                        // noise: keep it for the newest message and reveal the rest on hover.
                        visible: !wrapper.isStateEvent && (wrapper.index === 0 || wrapper.hovered)

                        eventId: wrapper.eventId
                        status: wrapper.status
                        trustlevel: wrapper.trustlevel
                        isEdited: wrapper.isEdited
                        isEncrypted: wrapper.isEncrypted
                        threadId: wrapper.threadId
                        timestamp: wrapper.timestamp
                        room: wrapper.room
                    }

                    Column {
                        id: contentColumn

                        anchors.left: parent.left
                        anchors.right: parent.right

                        AbstractButton {
                            id: replyRow
                            visible: wrapper.replyTo

                            leftPadding: Nheko.paddingSmall + 4

                            anchors.left: parent.left
                            anchors.right: parent.right

                            property color userColor: TimelineManager.userColor(wrapper.reply?.userId ?? '', palette.base)

                            clip: true

                            NhekoCursorShape {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                            }

                            contentItem: Column {
                                    spacing: 0

                                    id: replyCol

                                    AbstractButton {
                                        id: replyUserButton

                                        contentItem: Label {
                                            id: userName_
                                            text: wrapper.reply?.userName ?? 'missing name'
                                            color: replyRow.userColor
                                            textFormat: Text.RichText
                                            width: wrapper.maxWidth
                                            //elideWidth: wrapper.maxWidth
                                        }
                                        onClicked: wrapper.room.openUserProfile(wrapper.reply?.userId)
                                    }
                                    data: [
                                        replyUserButton,
                                        wrapper.reply,
                                    ]
                            }

                            background: Rectangle {
                                //width: replyRow.implicitContentWidth
                                color: Qt.tint(palette.base, Qt.hsla(replyRow.userColor.hslHue, 0.5, replyRow.userColor.hslLightness, 0.1))
                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    anchors.left: parent.left

                                    id: replyLine
                                    color: replyRow.userColor
                                    width: 4
                                }
                            }

                            onClicked: {
                                let link = wrapper.reply.hoveredLink
                                if (link) {
                                    Nheko.openLink(link)
                                } else {
                                    console.log("Scrolling to "+wrapper.replyTo);
                                    wrapper.room.showEvent(wrapper.replyTo)
                                }
                            }
                            onPressAndHold: wrapper.replyContextMenu.show(wrapper.reply.copyText ?? "", wrapper.reply.linkAt ? wrapper.reply.linkAt(pressX-replyLine.width - Nheko.paddingSmall, pressY - replyUserButton.implicitHeight) : "", wrapper.replyTo)
                            TapHandler {
                                acceptedButtons: Qt.RightButton
                                onSingleTapped: (eventPoint) => wrapper.replyContextMenu.show(wrapper.reply.copyText ?? "", wrapper.reply.linkAt ? wrapper.reply.linkAt(eventPoint.position.x-replyLine.width - Nheko.paddingSmall, eventPoint.position.y - replyUserButton.implicitHeight) : "", wrapper.replyTo)
                                gesturePolicy: TapHandler.ReleaseWithinBounds
                                acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                            }
                        }

                        data: [replyRow, wrapper.main]
                    }

                    // Column only manages the vertical stacking of its children, leaving x alone -
                    // wrapper.main defaults to x: 0 (flush left), which is fine for other senders'
                    // messages, but for our own (right-aligned) bubbles it means a short message
                    // grouped with a longer one has its slack space on the right, the same side as
                    // everyone else's messages. Mirroring it to hug the bubble's right edge instead
                    // puts that slack space on the left, where the action popup for our own
                    // messages belongs (see TimelineBubbleMessageStyle's
                    // textStartOffset/anchorFromRight).
                    // Kept as a sibling of contentColumn, not nested inside it: contentColumn
                    // already reparents wrapper.main into itself via the data list above, and a
                    // Binding declared as a further child of the same Column alongside that
                    // explicit data list assignment conflicts with it (wrapper.main ends up not
                    // reparented into contentColumn at all, rendering outside the bubble entirely).
                    // Set directly via x rather than anchors: wrapper.main isn't declared as
                    // contentColumn's child in QML (only placed there via that data list), so it
                    // isn't a valid anchor target/source pair with contentColumn (Qt Quick anchors
                    // only resolve between an item and its actual parent or sibling) - anchoring
                    // here silently fails and leaves the item unpositioned. Plain x has no such
                    // restriction.
                    Binding {
                        target: wrapper.main
                        property: "x"
                        // When the timestamp fits at the end of the text instead of getting its
                        // own row below (see fitsMetadata/ownContentWidth above), the bubble's
                        // content width already has metadata.width added on to make room for it.
                        // For left-aligned text that room falls after the text naturally; mirrored
                        // text has to explicitly stop short of the bubble's right edge by the same
                        // amount, or its right-aligned block sits flush against that edge and
                        // covers the timestamp instead of leaving it the room it was sized for.
                        value: wrapper.isSender ? Math.max(0, contentColumn.width - wrapper.main.width - ((contentPlacementContainer.fitsMetadata && !contentPlacementContainer.fitsMetadataInside) ? metadata.width : 0)) : 0
                        when: wrapper.main !== null
                    }
                }

                // Continuation lines within a group sit close together, like paragraphs in one
                // bubble; only the true start/end of a group gets the full roomy padding.
                topPadding: wrapper.isStateEvent ? 0 : (wrapper.isGroupStart ? Nheko.paddingSmall + 2 : 2)
                bottomPadding: wrapper.isStateEvent ? 0 : (wrapper.isGroupEnd ? Nheko.paddingSmall + 2 : 2)
                leftPadding: wrapper.isStateEvent ? 0 : Nheko.paddingMedium
                rightPadding: wrapper.isStateEvent ? 0 : Nheko.paddingMedium

                property color bubbleColor: wrapper.isStateEvent ? "transparent" : (wrapper.isSender ? Qt.tint(palette.base, Qt.hsla(palette.highlight.hslHue, wrapper.hovered ? 0.8 : 0.5, palette.highlight.hslLightness, 0.2)) : (wrapper.hovered ? palette.dark : palette.alternateBase))

                background: Rectangle {
                    color: messageBubble.bubbleColor
                    radius: 16
                    border.color: Nheko.theme.red
                    border.width: wrapper.notificationlevel == MtxEvent.Highlight ? 1 : 0

                    // Square off the corners on the side(s) where this bubble touches a
                    // neighboring message from the same sender, so a consecutive run reads as one
                    // continuous shape rather than a stack of separate bubbles.
                    Rectangle {
                        visible: !wrapper.isGroupStart && !wrapper.isStateEvent
                        color: parent.color
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: parent.radius
                    }
                    Rectangle {
                        visible: !wrapper.isGroupEnd && !wrapper.isStateEvent
                        color: parent.color
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: parent.radius
                    }
                }
            }

            DragHandler {
                id: replyDragHandler
                yAxis.enabled: false
                xAxis.enabled: true
                xAxis.minimum: wrapper.avatarMargin - 100
                xAxis.maximum: wrapper.avatarMargin
                onActiveChanged: {
                    if (!replyDragHandler.active) {
                        if (replyDragHandler.xAxis.minimum <= replyDragHandler.xAxis.activeValue + 1) {
                            wrapper.room.reply = wrapper.eventId
                        }
                        gridContainer.x = wrapper.avatarMargin;
                    }
                }
            }

            TapHandler {
                onDoubleTapped: wrapper.room.reply = wrapper.eventId
            }

        },
        Item {
            // We need this item to grab events, that otherwise would go to the TextArea in the main item. If we don't have this, it would trigger a right click menu on KDE...
            // https://invent.kde.org/frameworks/qqc2-desktop-style/-/blob/9d71fe874186009f76d392e203d9fa25a49f8be7/org.kde.desktop/TextArea.qml#L55
            
            anchors.fill: gridContainer
            anchors.topMargin: replyRow.height
            TapHandler {

                acceptedButtons: Qt.RightButton
                acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                gesturePolicy: TapHandler.ReleaseWithinBounds

                onSingleTapped: (event) => {
                    messageContextMenu.show(wrapper.eventId, wrapper.threadId, wrapper.type, wrapper.isSender, wrapper.isEncrypted, wrapper.isEditable, wrapper.main.hoveredLink, wrapper.main.copyText);
                }
            }
        },
        Reactions {
            id: reactionRow

            eventId: wrapper.eventId
            layoutDirection: (!wrapper.isStateEvent && wrapper.isSender) ? Qt.RightToLeft : Qt.LeftToRight
            reactions: wrapper.reactions
            bubbleColor: wrapper.bubble.bubbleColor
            width: wrapper.width - wrapper.avatarMargin
            x: wrapper.avatarMargin

            anchors {
                //left: row.bubbleOnRight ? undefined : row.left
                //right: row.bubbleOnRight ? row.right : undefined
                top: gridContainer.bottom
                topMargin: -4
            }
        },
        Rectangle {
            id: unreadRow

            color: palette.highlight
            height: visible ? 3 : 0
            visible: (wrapper.index > 0 && (wrapper.room.fullyReadEventId == wrapper.eventId))

            anchors {
                left: parent.left
                right: parent.right
                top: reactionRow.bottom
                topMargin: 5
            }
        }
    ]
}

