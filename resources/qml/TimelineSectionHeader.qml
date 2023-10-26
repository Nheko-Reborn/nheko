// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import im.nheko

import "./components"

Column {

    required property var day
    required property bool isSender
    required property bool isStateEvent
    required property int parentWidth
    required property var previousMessageDay
    required property bool previousMessageIsStateEvent
    required property string previousMessageUserId
    required property date timestamp
    required property string userId
    required property string userName
    required property string userPowerlevel

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

            PowerlevelIndicator {
                id: powerlevelIndicator
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter

                powerlevel: userPowerlevel
                height: fontMetrics.ascent
                width: height

                sourceSize.width: fontMetrics.lineSpacing
                sourceSize.height: fontMetrics.lineSpacing

                permissions: room ? room.permissions : null
                visible: isAdmin || isModerator
            }

            ToolTip.delay: Nheko.tooltipDelay
            ToolTip.text: userId
            ToolTip.visible: hovered
            leftPadding: powerlevelIndicator.visible ? 16 : 0
            leftInset: 0
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
            NhekoCursorShape {
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

