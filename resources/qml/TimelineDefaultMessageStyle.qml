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
    required property Item messageActions

    property int avatarMargin: (wrapper.isStateEvent || Settings.smallAvatars ? 0 : (Nheko.avatarSize + 8)) // align bubble with section header

    property alias hovered: messageHover.hovered
    property bool scrolledToThis: false

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
            anchors.top: gridContainer.top
            anchors.left: gridContainer.left 
            anchors.topMargin: -2
            anchors.leftMargin: wrapper.avatarMargin + 2
            color: "transparent"
            border.color: Nheko.theme.red
            border.width: wrapper.notificationlevel == MtxEvent.Highlight ? 1 : 0
            radius: 4
            height: contentColumn.implicitHeight + 4
            width: contentColumn.implicitWidth + 4
        },
        RowLayout {
            id: gridContainer

            width: wrapper.width
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
                        }
                    }
                }

            }

            Item {
                Layout.preferredWidth: wrapper.avatarMargin
            }

            AbstractButton {
                ToolTip.delay: Nheko.tooltipDelay
                ToolTip.text: qsTr("Part of a thread")
                ToolTip.visible: hovered
                Layout.fillHeight: true
                visible: wrapper.threadId
                Layout.preferredWidth: 4

                onClicked: wrapper.room.thread = wrapper.threadId

                Rectangle {
                    id: threadLine

                    anchors.fill: parent
                    color: TimelineManager.userColor(wrapper.threadId, palette.base)
                }
            }
            ColumnLayout {
                id: contentColumn
                Layout.fillWidth: true

                AbstractButton {
                    id: replyRow
                    visible: wrapper.reply
                    Layout.fillWidth: true
                    Layout.maximumHeight: timelineView.height / 8
                    Layout.preferredWidth: replyRowLay.implicitWidth
                    Layout.preferredHeight: replyRowLay.implicitHeight

                    property color userColor: TimelineManager.userColor(wrapper.reply?.userId ?? '', palette.base)

                    clip: true

                    NhekoCursorShape {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                    }

                    contentItem: RowLayout {
                        id: replyRowLay

                        anchors.fill: parent


                        Rectangle {
                            id: replyLine
                            Layout.fillHeight: true
                            color: replyRow.userColor
                            Layout.preferredWidth: 4
                        }

                        ColumnLayout {
                            spacing: 0

                            AbstractButton {
                                id: replyUserButton
                                Layout.fillWidth: true
                                contentItem: ElidedLabel {
                                    id: userName_
                                    fullText: wrapper.reply?.userName ?? ''
                                    color: replyRow.userColor
                                    textFormat: Text.RichText
                                    width: parent.width
                                    elideWidth: width
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
                        width: replyRow.implicitContentWidth
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
                }

                data: [
                    replyRow, wrapper.main,
                ]
            }

            Item {
                // spacer to fill width if needed
                Layout.fillWidth: true
            }

            RowLayout {
                id: metadata

                property int iconSize: Math.floor(fontMetrics.ascent * scaling)
                property double scaling: Settings.bubbles ? 0.75 : 1

                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                Layout.preferredWidth: implicitWidth
                spacing: 2
                visible: !isStateEvent

                StatusIndicator {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    eventId: wrapper.eventId
                    height: parent.iconSize
                    status: wrapper.status
                    width: parent.iconSize
                }
                Image {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Edited")
                    ToolTip.visible: editHovered.hovered
                    height: parent.iconSize
                    source: "image://colorimage/:/icons/icons/ui/edit.svg?" + ((wrapper.eventId == wrapper.room.edit) ? palette.highlight : palette.buttonText)
                    sourceSize.height: parent.iconSize * Screen.devicePixelRatio
                    sourceSize.width: parent.iconSize * Screen.devicePixelRatio
                    visible: wrapper.isEdited || wrapper.eventId == wrapper.room.edit
                    width: parent.iconSize

                    HoverHandler {
                        id: editHovered

                    }
                }
                ImageButton {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Part of a thread")
                    ToolTip.visible: hovered
                    buttonTextColor: TimelineManager.userColor(wrapper.threadId, palette.base)
                    height: parent.iconSize
                    image: ":/icons/icons/ui/thread.svg"
                    visible: wrapper.threadId
                    width: parent.iconSize

                    onClicked: wrapper.room.thread = threadId
                }
                EncryptionIndicator {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    encrypted: wrapper.isEncrypted
                    height: parent.iconSize
                    sourceSize.height: parent.iconSize * Screen.devicePixelRatio
                    sourceSize.width: parent.iconSize * Screen.devicePixelRatio
                    trust: wrapper.trustlevel
                    visible: wrapper.room.isEncrypted
                    width: parent.iconSize
                }
                Label {
                    id: ts

                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    Layout.preferredWidth: implicitWidth
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: Qt.formatDateTime(wrapper.timestamp, Qt.DefaultLocaleLongDate)
                    ToolTip.visible: ma.hovered
                    color: palette.inactive.text
                    font.pointSize: fontMetrics.font.pointSize * parent.scaling
                    text: wrapper.timestamp.toLocaleTimeString(Locale.ShortFormat)

                    HoverHandler {
                        id: ma

                    }
                }
            }
        },
        Reactions {
            id: reactionRow

            eventId: wrapper.eventId
            layoutDirection: row.bubbleOnRight ? Qt.RightToLeft : Qt.LeftToRight
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
