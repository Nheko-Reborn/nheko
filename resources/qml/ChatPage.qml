// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import "components"
import im.nheko 1.0

// this needs to be last
import QtQml 2.15

Rectangle {
    id: chatPage

    color: palette.window

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: offlineIndicator

            Layout.fillWidth: true
            Layout.preferredHeight: offlineLabel.height + Nheko.paddingMedium
            color: Nheko.theme.error
            visible: !TimelineManager.isConnected
            z: 1

            Label {
                id: offlineLabel

                anchors.centerIn: parent
                text: qsTr("No network connection")
            }
        }
        AdaptiveLayout {
            id: adaptiveView

            function initializePageIndex() {
                if (!singlePageMode)
                    adaptiveView.pageIndex = 0;
                else if (Rooms.currentRoom || Rooms.currentRoomPreview.roomid)
                    adaptiveView.pageIndex = 2;
                else
                    adaptiveView.pageIndex = 1;
            }

            Layout.fillHeight: true
            Layout.fillWidth: true
            pageIndex: 1
            singlePageMode: communityListC.preferredWidth + roomListC.preferredWidth + timlineViewC.minimumWidth > width

            Component.onCompleted: initializePageIndex()
            onSinglePageModeChanged: initializePageIndex()

            Connections {
                function onCurrentRoomChanged() {
                    adaptiveView.initializePageIndex();
                }

                target: Rooms
            }
            AdaptiveLayoutElement {
                id: communityListC

                collapsedWidth: communitiesList.avatarSize + 2 * Nheko.paddingMedium
                maximumWidth: communitiesList.avatarSize * 10 + 2 * Nheko.paddingMedium
                minimumWidth: communitiesList.avatarSize * 4 + Nheko.paddingMedium * 2
                preferredWidth: Settings.communityListWidth >= minimumWidth ? Settings.communityListWidth : collapsedWidth
                visible: Settings.groupView

                CommunitiesList {
                    id: communitiesList

                    collapsed: parent.collapsed
                }
                Binding {
                    delayed: true
                    property: 'communityListWidth'
                    restoreMode: Binding.RestoreBindingOrValue
                    target: Settings
                    value: communityListC.preferredWidth
                    when: !adaptiveView.singlePageMode
                }
            }
            AdaptiveLayoutElement {
                id: roomListC

                collapsedWidth: roomlist.avatarSize + 2 * Nheko.paddingMedium
                maximumWidth: roomlist.avatarSize * 10 + Nheko.paddingSmall * 2
                minimumWidth: roomlist.avatarSize * 4 + Nheko.paddingSmall * 2
                preferredWidth: (Settings.roomListWidth == -1) ? (roomlist.avatarSize * 5 + Nheko.paddingSmall * 2) : (Settings.roomListWidth >= minimumWidth ? Settings.roomListWidth : collapsedWidth)

                RoomList {
                    id: roomlist

                    collapsed: parent.collapsed
                    height: adaptiveView.height
                }
                Binding {
                    delayed: true
                    property: 'roomListWidth'
                    restoreMode: Binding.RestoreBindingOrValue
                    target: Settings
                    value: roomListC.preferredWidth
                    when: !adaptiveView.singlePageMode
                }
            }
            AdaptiveLayoutElement {
                id: timlineViewC

                minimumWidth: fontMetrics.averageCharacterWidth * 40 + Nheko.avatarSize + 2 * Nheko.paddingMedium

                TimelineView {
                    id: timeline

                    privacyScreen: privacyScreen
                    room: Rooms.currentRoom
                    roomPreview: Rooms.currentRoomPreview.roomid ? Rooms.currentRoomPreview : null
                    showBackButton: adaptiveView.singlePageMode
                }
            }
        }
    }
    PrivacyScreen {
        id: privacyScreen

        anchors.fill: parent
        screenTimeout: Settings.privacyScreenTimeout
        timelineRoot: adaptiveView
        visible: Settings.privacyScreen
        windowTarget: MainWindow
    }
}
