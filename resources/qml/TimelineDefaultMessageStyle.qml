// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import im.nheko

TimelineEvent {
    id: wrapper

    property int avatarMargin: (wrapper.isStateEvent || Settings.smallAvatars ? 0 : (Nheko.avatarSize + 8)) // align bubble with section header

    //room: chatRoot.roommodel

    required property var day
    property alias hovered: messageHover.hovered
    required property int index
    required property bool isEditable
    required property bool isEdited
    required property bool isEncrypted
    required property bool isSender
    required property Item messageActions
    required property QtObject messageContextMenu
    required property int notificationlevel
    property var previousMessageDay: (index + 1) >= chat.count ? 0 : chat.model.dataByIndex(index + 1, Room.Day)
    property bool previousMessageIsStateEvent: (index + 1) >= chat.count ? true : chat.model.dataByIndex(index + 1, Room.IsStateEvent)
    property string previousMessageUserId: (index + 1) >= chat.count ? "" : chat.model.dataByIndex(index + 1, Room.UserId)
    required property var reactions
    required property QtObject replyContextMenu
    property bool scrolledToThis: false
    required property int status
    required property string threadId
    required property date timestamp
    required property int trustlevel
    required property int type
    required property string userId
    required property string userName
    required property int userPowerlevel

    ListView.delayRemove: true
    anchors.horizontalCenter: ListView.view.contentItem.horizontalCenter
    height: Math.max((section.item?.height ?? 0) + gridContainer.implicitHeight + reactionRow.implicitHeight + unreadRow.height, 10)
    mainInset: (threadId ? (4 + Nheko.paddingSmall) : 0)
    maxWidth: chat.delegateMaxWidth - avatarMargin - metadata.width
    replyInset: mainInset + 4 + Nheko.paddingSmall
    width: chat.delegateMaxWidth

    data: [
        Loader {
            id: section

            active: wrapper.previousMessageUserId !== wrapper.userId || wrapper.previousMessageDay !== wrapper.day || wrapper.previousMessageIsStateEvent !== wrapper.isStateEvent
            visible: status == Loader.Ready
            z: 4

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
        },
        Rectangle {
            anchors.fill: gridContainer
            color: (Settings.messageHoverHighlight && messageHover.hovered) ? palette.alternateBase : "transparent"

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
        Rectangle {
            anchors.left: gridContainer.left
            anchors.leftMargin: -2
            anchors.top: gridContainer.top
            anchors.topMargin: -2
            border.color: Nheko.theme.red
            border.width: wrapper.notificationlevel == MtxEvent.Highlight ? 1 : 0
            color: "transparent"
            height: contentColumn.implicitHeight + 4
            radius: 4
            width: contentColumn.implicitWidth + 4
        },
        Row {
            id: gridContainer

            spacing: Nheko.paddingSmall
            width: wrapper.width - wrapper.avatarMargin
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
                            messageActions.anchors.bottomMargin = -gridContainer.y;
                            messageActions.anchors.rightMargin = metadata.width;
                        }
                    }
                }
            }
            AbstractButton {
                ToolTip.delay: Nheko.tooltipDelay
                ToolTip.text: qsTr("Part of a thread")
                ToolTip.visible: hovered
                height: contentColumn.height
                visible: wrapper.threadId
                width: 4

                onClicked: wrapper.room.thread = wrapper.threadId

                Rectangle {
                    id: threadLine

                    anchors.fill: parent
                    color: TimelineManager.userColor(wrapper.threadId, palette.base)
                }
            }
            Item {
                height: 1
                visible: wrapper.isStateEvent
                width: (wrapper.maxWidth - (wrapper.main?.width ?? 0)) / 2
            }
            Column {
                id: contentColumn

                data: [replyRow, wrapper.main,]

                AbstractButton {
                    id: replyRow

                    property color userColor: TimelineManager.userColor(wrapper.reply?.userId ?? '', palette.base)

                    clip: true
                    height: replyLine.height
                    visible: wrapper.reply

                    background: Rectangle {
                        //width: replyRow.implicitContentWidth
                        color: Qt.tint(palette.base, Qt.hsla(replyRow.userColor.hslHue, 0.5, replyRow.userColor.hslLightness, 0.1))
                    }
                    contentItem: Row {
                        id: replyRowLay

                        spacing: Nheko.paddingSmall

                        Rectangle {
                            id: replyLine

                            color: replyRow.userColor
                            height: Math.min(wrapper.reply?.height, timelineView.height / 10) + Nheko.paddingSmall + replyUserButton.height
                            width: 4
                        }
                        Column {
                            id: replyCol

                            data: [replyUserButton, wrapper.reply,]
                            spacing: 0

                            AbstractButton {
                                id: replyUserButton

                                contentItem: Label {
                                    id: userName_

                                    color: replyRow.userColor
                                    text: wrapper.reply?.userName ?? ''
                                    textFormat: Text.RichText
                                    width: wrapper.maxWidth
                                    //elideWidth: wrapper.maxWidth
                                }

                                onClicked: wrapper.room.openUserProfile(wrapper.reply?.userId)
                            }
                        }
                    }

                    onClicked: {
                        let link = wrapper.reply.hoveredLink;
                        if (link) {
                            Nheko.openLink(link);
                        } else {
                            console.log("Scrolling to " + wrapper.replyTo);
                            wrapper.room.showEvent(wrapper.replyTo);
                        }
                    }
                    onPressAndHold: wrapper.replyContextMenu.show(wrapper.reply.copyText ?? "", wrapper.reply.linkAt ? wrapper.reply.linkAt(pressX - replyLine.width - Nheko.paddingSmall, pressY - replyUserButton.implicitHeight) : "", wrapper.replyTo)

                    NhekoCursorShape {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                    }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                        gesturePolicy: TapHandler.ReleaseWithinBounds

                        onSingleTapped: eventPoint => wrapper.replyContextMenu.show(wrapper.reply.copyText ?? "", wrapper.reply.linkAt ? wrapper.reply.linkAt(eventPoint.position.x - replyLine.width - Nheko.paddingSmall, eventPoint.position.y - replyUserButton.implicitHeight) : "", wrapper.replyTo)
                    }
                }
            }
            DragHandler {
                id: replyDragHandler

                xAxis.enabled: true
                xAxis.maximum: wrapper.avatarMargin
                xAxis.minimum: wrapper.avatarMargin - 100
                yAxis.enabled: false

                onActiveChanged: {
                    if (!replyDragHandler.active) {
                        if (replyDragHandler.xAxis.minimum <= replyDragHandler.xAxis.activeValue + 1) {
                            wrapper.room.reply = wrapper.eventId;
                        }
                        gridContainer.x = wrapper.avatarMargin;
                    }
                }
            }
            TapHandler {
                onDoubleTapped: wrapper.room.reply = wrapper.eventId
            }
        },
        TimelineMetadata {
            id: metadata

            anchors.right: parent.right
            eventId: wrapper.eventId
            isEdited: wrapper.isEdited
            isEncrypted: wrapper.isEncrypted
            room: wrapper.room
            scaling: 1
            status: wrapper.status
            threadId: wrapper.threadId
            timestamp: wrapper.timestamp
            trustlevel: wrapper.trustlevel
            visible: !wrapper.isStateEvent
            y: section.visible && section.active ? section.y + section.height : 0
        },
        Reactions {
            id: reactionRow

            eventId: wrapper.eventId
            reactions: wrapper.reactions
            width: wrapper.width - wrapper.avatarMargin
            x: wrapper.avatarMargin

            anchors {
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
