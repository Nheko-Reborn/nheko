// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
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

    color: Nheko.colors.window

    ColumnLayout {
        spacing: 0
        anchors.fill: parent

        Rectangle {
            id: offlineIndicator

            color: Nheko.theme.error
            visible: !TimelineManager.isConnected
            Layout.preferredHeight: offlineLabel.height + Nheko.paddingMedium
            Layout.fillWidth: true
            z: 1

            Label {
                id: offlineLabel

                anchors.centerIn: parent
                text: qsTr("No network connection")
            }
        }

        AdaptiveLayout {
            id: adaptiveView

            Layout.fillWidth: true
            Layout.fillHeight: true
            singlePageMode: communityListC.preferredWidth + roomListC.preferredWidth + timlineViewC.minimumWidth > width
            pageIndex: 1

            Component.onCompleted: initializePageIndex()
            onSinglePageModeChanged: initializePageIndex()

            function initializePageIndex() {
                if (!singlePageMode)
                    adaptiveView.pageIndex = 0;
                else if (Rooms.currentRoom || Rooms.currentRoomPreview.roomid)
                    adaptiveView.pageIndex = 2;
                else
                    adaptiveView.pageIndex = 1;
            }

            Connections {
                target: Rooms
                function onCurrentRoomChanged() {
                    adaptiveView.initializePageIndex();
                }
            }

            AdaptiveLayoutElement {
                id: communityListC

                visible: Settings.groupView
                minimumWidth: communitiesList.avatarSize * 4 + Nheko.paddingMedium * 2
                collapsedWidth: communitiesList.avatarSize + 2 * Nheko.paddingMedium
                preferredWidth: Settings.communityListWidth >= minimumWidth ? Settings.communityListWidth : collapsedWidth
                maximumWidth: communitiesList.avatarSize * 10 + 2 * Nheko.paddingMedium

                CommunitiesList {
                    id: communitiesList

                    collapsed: parent.collapsed
                }

                Binding {
                    target: Settings
                    property: 'communityListWidth'
                    value: communityListC.preferredWidth
                    when: !adaptiveView.singlePageMode
                    delayed: true
                    restoreMode: Binding.RestoreBindingOrValue
                }

            }

            AdaptiveLayoutElement {
                id: roomListC

                minimumWidth: roomlist.avatarSize * 4 + Nheko.paddingSmall * 2
                preferredWidth: (Settings.roomListWidth == - 1)
                    ? (roomlist.avatarSize * 5 + Nheko.paddingSmall * 2)
                    : (Settings.roomListWidth >= minimumWidth ? Settings.roomListWidth : collapsedWidth)
                maximumWidth: roomlist.avatarSize * 10 + Nheko.paddingSmall * 2
                collapsedWidth: roomlist.avatarSize + 2 * Nheko.paddingMedium

                RoomList {
                    id: roomlist

                    height: adaptiveView.height
                    collapsed: parent.collapsed
                }

                Binding {
                    target: Settings
                    property: 'roomListWidth'
                    value: roomListC.preferredWidth
                    when: !adaptiveView.singlePageMode
                    delayed: true
                    restoreMode: Binding.RestoreBindingOrValue
                }

            }

            AdaptiveLayoutElement {
                id: timlineViewC

                minimumWidth: fontMetrics.averageCharacterWidth * 40 + Nheko.avatarSize + 2 * Nheko.paddingMedium

                TimelineView {
                    id: timeline

                    showBackButton: adaptiveView.singlePageMode
                    room: Rooms.currentRoom
                    roomPreview: Rooms.currentRoomPreview.roomid ? Rooms.currentRoomPreview : null
                }

            }

        }

    }

    PrivacyScreen {
        anchors.fill: parent
        visible: Settings.privacyScreen
        screenTimeout: Settings.privacyScreenTimeout
        timelineRoot: adaptiveView
        windowTarget: MainWindow
    }

}
