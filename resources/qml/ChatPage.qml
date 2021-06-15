// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import "components"
import im.nheko 1.0

Rectangle {
    id: chatPage

    color: Nheko.colors.window

    AdaptiveLayout {
        id: adaptiveView

        anchors.fill: parent
        singlePageMode: communityListC.preferredWidth + roomListC.preferredWidth + timlineViewC.minimumWidth > width
        pageIndex: Rooms.currentRoom ? 2 : 1

        AdaptiveLayoutElement {
            id: communityListC

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
            }

        }

        AdaptiveLayoutElement {
            id: roomListC

            minimumWidth: roomlist.avatarSize * 4 + Nheko.paddingSmall * 2
            preferredWidth: Settings.roomListWidth >= minimumWidth ? Settings.roomListWidth : roomlist.avatarSize * 5 + Nheko.paddingSmall * 2
            maximumWidth: roomlist.avatarSize * 10 + Nheko.paddingSmall * 2
            collapsedWidth: roomlist.avatarSize + 2 * Nheko.paddingMedium

            RoomList {
                id: roomlist

                collapsed: parent.collapsed
            }

            Binding {
                target: Settings
                property: 'roomListWidth'
                value: roomListC.preferredWidth
                when: !adaptiveView.singlePageMode
                delayed: true
            }

        }

        AdaptiveLayoutElement {
            id: timlineViewC

            minimumWidth: fontMetrics.averageCharacterWidth * 40 + Nheko.avatarSize + 2 * Nheko.paddingMedium

            TimelineView {
                id: timeline

                showBackButton: adaptiveView.singlePageMode
                room: Rooms.currentRoom
            }

        }

    }

    PrivacyScreen {
        anchors.fill: parent
        visible: Settings.privacyScreen
        screenTimeout: Settings.privacyScreenTimeout
        timelineRoot: adaptiveView
    }

}
