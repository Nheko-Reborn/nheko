// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./components"
import "./delegates"
import "./emoji"
import "./ui"
import "./dialogs"
import Qt.labs.platform 1.1 as Platform
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import im.nheko

TimelineEvent {
    id: wrapper
    ListView.delayRemove: true
    width: chat.delegateMaxWidth
    height: Math.max((section.item?.height ?? 0) + gridContainer.implicitHeight + reactionRow.implicitHeight + unreadRow.height, 10)
    anchors.horizontalCenter: ListView.view.contentItem.horizontalCenter
    //room: chatRoot.roommodel

    required property var day
    required property bool isSender
    required property int index
    property var previousMessageDay: (index + 1) >= chat.count ? 0 : chat.model.dataByIndex(index + 1, Room.Day)
    property bool previousMessageIsStateEvent: (index + 1) >= chat.count ? true : chat.model.dataByIndex(index + 1, Room.IsStateEvent)
    property string previousMessageUserId: (index + 1) >= chat.count ? "" : chat.model.dataByIndex(index + 1, Room.UserId)

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

    property alias hovered: messageHover.hovered
    property bool scrolledToThis: false

    mainInset: (threadId ? (4 + Nheko.paddingSmall) : 0) + 4
    replyInset: mainInset + 4 + Nheko.paddingSmall

    property int bubbleMargin: 40

    maxWidth: chat.delegateMaxWidth - avatarMargin - bubbleMargin

    data: [
        Loader {
            id: section

            active: wrapper.previousMessageUserId !== wrapper.userId || wrapper.previousMessageDay !== wrapper.day || wrapper.previousMessageIsStateEvent !== wrapper.isStateEvent
            //asynchronous: true
            sourceComponent: TimelineSectionHeader {
                day: wrapper.day
                isSender: wrapper.isSender
                isStateEvent: wrapper.isStateEvent
                parentWidth: wrapper.width
                previousMessageDay: wrapper.previousMessageDay
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
                    if (!Settings.mobileMode && hovered) {
                        if (!messageActions.hovered) {
                            messageActions.model = wrapper;
                            messageActions.attached = wrapper;
                            messageActions.anchors.bottomMargin = -gridContainer.y
                            //messageActions.anchors.rightMargin = metadata.width
                        }
                    }
                }

            }


            AbstractButton {
                id: messageBubble

                anchors.left: (wrapper.isStateEvent || wrapper.isSender) ? undefined : parent.left
                anchors.right: (wrapper.isStateEvent || !wrapper.isSender) ? undefined : parent.right
                anchors.horizontalCenter: wrapper.isStateEvent ? parent.horizontalCenter : undefined

                property color userColor: TimelineManager.userColor(wrapper.main?.userId ?? '', palette.base)

                contentItem: Item {
                    id: contentPlacementContainer

                    property bool fitsMetadata: ((wrapper.main?.width ?? 0) + wrapper.mainInset + metadata.width) < wrapper.maxWidth

                    // This doesnt work because of tables. They might have content in the top of the cell, while the background reaches to the bottom. Maybe using the textDocument we could do more?
                    // property bool fitsMetadataInside: wrapper.main?.positionAt ? (wrapper.main.positionAt(wrapper.main.width, wrapper.main.height - 4) == wrapper.main.positionAt(wrapper.main.width - metadata.width, wrapper.main.height - 4)) : false
                    property bool fitsMetadataInside: false

                    implicitWidth: Math.max((wrapper.reply?.width ?? 0) + wrapper.replyInset, (wrapper.main?.width ?? 0) + wrapper.mainInset + ((fitsMetadata && !fitsMetadataInside) ? metadata.width : 0))
                    implicitHeight: contentColumn.implicitHeight + ((fitsMetadata || fitsMetadataInside) ? 0 : metadata.height)

                    TimelineMetadata {
                        id: metadata

                        scaling: 0.75

                        anchors.right: parent.right
                        anchors.bottom: parent.bottom

                        visible: !wrapper.isStateEvent

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
                            visible: wrapper.reply

                            height: replyLine.height
                            anchors.left: parent.left
                            anchors.right: parent.right

                            property color userColor: TimelineManager.userColor(wrapper.reply?.userId ?? '', palette.base)

                            clip: true

                            NhekoCursorShape {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                            }

                            contentItem: Row {
                                id: replyRowLay

                                spacing: Nheko.paddingSmall

                                Rectangle {
                                    id: replyLine
                                    height: Math.min( wrapper.reply?.height, timelineView.height / 10) + Nheko.paddingSmall + replyUserButton.height
                                    color: replyRow.userColor
                                    width: 4
                                }

                                Column {
                                    spacing: 0

                                    id: replyCol

                                    AbstractButton {
                                        id: replyUserButton

                                        contentItem: Label {
                                            id: userName_
                                            text: wrapper.reply?.userName ?? ''
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
                            }

                            background: Rectangle {
                                //width: replyRow.implicitContentWidth
                                color: Qt.tint(palette.base, Qt.hsla(replyRow.userColor.hslHue, 0.5, replyRow.userColor.hslLightness, 0.1))
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
                }

                padding: wrapper.isStateEvent ? 0 : 4
                background: Rectangle {
                    color: !wrapper.isStateEvent ? Qt.tint(palette.base, Qt.hsla(messageBubble.userColor.hslHue, wrapper.hovered ? 0.8 : 0.5, messageBubble.userColor.hslLightness, 0.2)) : "transparent"
                    radius: 4
                    border.color: Nheko.theme.red
                    border.width: wrapper.notificationlevel == MtxEvent.Highlight ? 1 : 0
                }
            }
        },
        Reactions {
            id: reactionRow

            eventId: wrapper.eventId
            layoutDirection: (!wrapper.isStateEvent && wrapper.isSender) ? Qt.RightToLeft : Qt.LeftToRight
            reactions: wrapper.reactions
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

