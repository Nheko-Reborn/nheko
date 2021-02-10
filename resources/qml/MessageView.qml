import "./delegates"
import QtGraphicalEffects 1.0
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import im.nheko 1.0

ListView {
    id: chat

    property int delegateMaxWidth: (Settings.timelineMaxWidth > 100 && (parent.width - Settings.timelineMaxWidth) > scrollbar.width * 2) ? Settings.timelineMaxWidth : (parent.width - scrollbar.width * 2 - 8)

    Layout.fillWidth: true
    Layout.fillHeight: true
    cacheBuffer: 400
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
            if (chat.model.edit)
                chat.model.edit = undefined;
            else
                chat.model.reply = undefined;
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

                    MouseArea {
                        anchors.fill: parent
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: chat.model.openUserProfile(modelData.userId)
                        cursorShape: Qt.PointingHandCursor
                        propagateComposedEvents: true
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

    ScrollBar.vertical: ScrollBar {
        id: scrollbar
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

            y: section.active && section.visible ? section.y + section.height : 0
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
