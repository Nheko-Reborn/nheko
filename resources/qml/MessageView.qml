// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates"
import "./emoji"
import QtGraphicalEffects 1.0
import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import im.nheko 1.0

ScrollView {
    clip: false
    palette: colors
    padding: 8

    ListView {
        id: chat

        property int delegateMaxWidth: ((Settings.timelineMaxWidth > 100 && Settings.timelineMaxWidth < parent.availableWidth) ? Settings.timelineMaxWidth : parent.availableWidth) - parent.padding * 2

        model: TimelineManager.timeline
        boundsBehavior: Flickable.StopAtBounds
        pixelAligned: true
        spacing: 4
        verticalLayoutDirection: ListView.BottomToTop
        onCountChanged: {
            // Mark timeline as read
            if (atYEnd)
                model.currentIndex = 0;

        }

        Rectangle {
            //closePolicy: Popup.NoAutoClose

            id: messageActions

            property Item attached: null
            property alias model: row.model
            // use comma to update on scroll
            property var attachedPos: chat.contentY, attached ? chat.mapFromItem(attached, attached ? attached.width - width : 0, -height) : null
            readonly property int padding: 4

            visible: Settings.buttonsInTimeline && !!attached && (attached.hovered || messageActionHover.hovered)
            x: attached ? attachedPos.x : 0
            y: attached ? attachedPos.y : 0
            z: 10
            height: row.implicitHeight + padding * 2
            width: row.implicitWidth + padding * 2
            color: colors.window
            border.color: colors.buttonText
            border.width: 1
            radius: padding

            HoverHandler {
                id: messageActionHover

                grabPermissions: PointerHandler.CanTakeOverFromAnything
            }

            Row {
                id: row

                property var model

                anchors.centerIn: parent
                spacing: messageActions.padding

                ImageButton {
                    id: editButton

                    visible: !!row.model && row.model.isEditable
                    buttonTextColor: colors.buttonText
                    width: 16
                    hoverEnabled: true
                    image: ":/icons/icons/ui/edit.png"
                    ToolTip.visible: hovered
                    ToolTip.text: row.model && row.model.isEditable ? qsTr("Edit") : qsTr("Edited")
                    onClicked: {
                        if (row.model.isEditable)
                            chat.model.editAction(row.model.id);

                    }
                }

                EmojiButton {
                    id: reactButton

                    width: 16
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("React")
                    emojiPicker: emojiPopup
                    event_id: row.model ? row.model.id : ""
                }

                ImageButton {
                    id: replyButton

                    width: 16
                    hoverEnabled: true
                    image: ":/icons/icons/ui/mail-reply.png"
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Reply")
                    onClicked: chat.model.replyAction(row.model.id)
                }

                ImageButton {
                    id: optionsButton

                    width: 16
                    hoverEnabled: true
                    image: ":/icons/icons/ui/vertical-ellipsis.png"
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Options")
                    onClicked: messageContextMenu.show(row.model.id, row.model.type, row.model.isEncrypted, row.model.isEditable, optionsButton)
                }

            }

        }

        ScrollHelper {
            flickable: parent
            anchors.fill: parent
            enabled: !Settings.mobileMode
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
            onActivated: {
                if (chat.model.reply)
                    chat.model.reply = undefined;
                else
                    chat.model.edit = undefined;
            }
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

        Shortcut {
            sequence: "Ctrl+E"
            onActivated: {
                chat.model.edit = chat.model.reply;
            }
        }

        Connections {
            target: TimelineManager
            onFocusChanged: readTimer.running = TimelineManager.isWindowFocused
        }

        Timer {
            id: readTimer

            // force current read index to update
            onTriggered: chat.model.setCurrentIndex(chat.model.currentIndex)
            interval: 1000
        }

        Component {
            id: sectionHeader

            Column {
                topPadding: 4
                bottomPadding: 4
                spacing: 8
                visible: modelData && (modelData.previousMessageUserId !== modelData.userId || modelData.previousMessageDay !== modelData.day)
                width: parentWidth
                height: ((modelData && modelData.previousMessageDay !== modelData.day) ? dateBubble.height + 8 + userName.height : userName.height) + 8

                Label {
                    id: dateBubble

                    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                    visible: modelData && modelData.previousMessageDay !== modelData.day
                    text: modelData ? chat.model.formatDateSeparator(modelData.timestamp) : ""
                    color: colors.text
                    height: Math.round(fontMetrics.height * 1.4)
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
                        id: messageUserAvatar

                        width: avatarSize
                        height: avatarSize
                        url: modelData ? chat.model.avatarUrl(modelData.userId).replace("mxc://", "image://MxcImage/") : ""
                        displayName: modelData ? modelData.userName : ""
                        userid: modelData ? modelData.userId : ""
                        onClicked: chat.model.openUserProfile(modelData.userId)
                        ToolTip.visible: avatarHover.hovered
                        ToolTip.text: userid

                        HoverHandler {
                            id: avatarHover
                        }

                    }

                    Connections {
                        target: chat.model
                        onRoomAvatarUrlChanged: {
                            messageUserAvatar.url = modelData ? chat.model.avatarUrl(modelData.userId).replace("mxc://", "image://MxcImage/") : "";
                        }
                    }

                    Label {
                        id: userName

                        text: modelData ? TimelineManager.escapeEmoji(modelData.userName) : ""
                        color: TimelineManager.userColor(modelData ? modelData.userId : "", colors.window)
                        textFormat: Text.RichText
                        ToolTip.visible: displayNameHover.hovered
                        ToolTip.text: modelData ? modelData.userId : ""

                        TapHandler {
                            onSingleTapped: chat.model.openUserProfile(modelData.userId)
                        }

                        CursorShape {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                        }

                        HoverHandler {
                            id: displayNameHover
                        }

                    }

                    Label {
                        color: colors.buttonText
                        text: modelData ? TimelineManager.userStatus(modelData.userId) : ""
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                        width: chat.delegateMaxWidth - parent.spacing * 2 - userName.implicitWidth - avatarSize
                        font.italic: true
                    }

                }

            }

        }

        delegate: Item {
            id: wrapper

            anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
            width: chat.delegateMaxWidth
            height: section ? section.height + timelinerow.height : timelinerow.height

            Loader {
                id: section

                property var modelData: model
                property int parentWidth: parent.width

                active: model.previousMessageUserId !== undefined && model.previousMessageUserId !== model.userId || model.previousMessageDay !== model.day
                //asynchronous: true
                sourceComponent: sectionHeader
                visible: status == Loader.Ready
            }

            TimelineRow {
                id: timelinerow

                property alias hovered: hoverHandler.hovered

                y: section.visible && section.active ? section.y + section.height : 0

                HoverHandler {
                    id: hoverHandler

                    enabled: !Settings.mobileMode
                    onHoveredChanged: {
                        if (hovered) {
                            if (!messageActionHover.hovered) {
                                messageActions.attached = timelinerow;
                                messageActions.model = model;
                            }
                        }
                    }
                }

            }

            Connections {
                target: chat
                onMovementEnded: {
                    if (y + height + 2 * chat.spacing > chat.contentY + chat.height && y < chat.contentY + chat.height)
                        chat.model.currentIndex = index;

                }
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
