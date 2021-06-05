// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.3
import "components"
import im.nheko 1.0

Rectangle {
    id: chatPage

    color: Nheko.colors.window

    AdaptiveLayout {
        anchors.fill: parent
        singlePageMode: width < communityListC.maximumWidth + roomListC.maximumWidth + timlineViewC.minimumWidth

        AdaptiveLayoutElement {
            id: communityListC

            minimumWidth: Nheko.avatarSize * 2 + Nheko.paddingSmall * 2
            collapsedWidth: Nheko.avatarSize + Nheko.paddingSmall * 2
            preferredWidth: Nheko.avatarSize + Nheko.paddingSmall * 2
            maximumWidth: Nheko.avatarSize * 7 + Nheko.paddingSmall * 2

            Rectangle {
                color: Nheko.theme.sidebarBackground
            }

        }

        AdaptiveLayoutElement {
            id: roomListC

            minimumWidth: Nheko.avatarSize * 5 + Nheko.paddingSmall * 2
            preferredWidth: Nheko.avatarSize * 5 + Nheko.paddingSmall * 2
            maximumWidth: Nheko.avatarSize * 10 + Nheko.paddingSmall * 2
            collapsedWidth: Nheko.avatarSize + Nheko.paddingSmall * 2

            RoomList {
            }

        }

        AdaptiveLayoutElement {
            id: timlineViewC

            minimumWidth: 400

            TimelineView {
                id: timeline

                room: Rooms.currentRoom
            }

        }

    }

    PrivacyScreen {
        anchors.fill: parent
        visible: Settings.privacyScreen
        screenTimeout: Settings.privacyScreenTimeout
        timelineRoot: timeline
    }

}
