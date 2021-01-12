import "./delegates"
import QtGraphicalEffects 1.0
import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10
import QtQuick.Window 2.10
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
        if (atYEnd)
            model.currentIndex = 0;

    } // Mark last event as read, since we are at the bottom

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
        onActivated: chat.model.reply = undefined
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

    section {
        property: "section"
    }

    Component {
        id: sectionHeader

        Column {
            property var modelData
            property string section
            property string nextSection

            topPadding: 4
            bottomPadding: 4
            spacing: 8
            visible: !!modelData
            width: parent.width
            height: (section.includes(" ") ? dateBubble.height + 8 + userName.height : userName.height) + 8

            Label {
                id: dateBubble

                anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                visible: section.includes(" ")
                text: chat.model.formatDateSeparator(modelData.timestamp)
                color: colors.text
                height: fontMetrics.height * 1.4
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
                    // MouseArea {
                    //     anchors.fill: parent
                    //     onClicked: chat.model.openUserProfile(modelData.userId)
                    //     cursorShape: Qt.PointingHandCursor
                    //     propagateComposedEvents: true
                    // }

                    width: avatarSize
                    height: avatarSize
                    url: chat.model.avatarUrl(modelData.userId).replace("mxc://", "image://MxcImage/")
                    displayName: modelData.userName
                    userid: modelData.userId
                    onClicked: chat.model.openUserProfile(modelData.userId)
                }

                Label {
                    id: userName

                    text: TimelineManager.escapeEmoji(modelData.userName)
                    color: TimelineManager.userColor(modelData.userId, colors.window)
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
                    text: TimelineManager.userStatus(modelData.userId)
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

        // This would normally be previousSection, but our model's order is inverted.
        property bool sectionBoundary: (ListView.nextSection != "" && ListView.nextSection !== ListView.section) || model.index === chat.count - 1
        property Item section

        anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
        width: chat.delegateMaxWidth
        height: section ? section.height + timelinerow.height : timelinerow.height
        onSectionBoundaryChanged: {
            if (sectionBoundary) {
                var properties = {
                    "modelData": model.dump,
                    "section": ListView.section,
                    "nextSection": ListView.nextSection
                };
                section = sectionHeader.createObject(wrapper, properties);
            } else {
                section.destroy();
                section = null;
            }
        }

        TimelineRow {
            id: timelinerow

            y: section ? section.y + section.height : 0
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
