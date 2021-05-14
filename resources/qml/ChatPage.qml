// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.3
import im.nheko 1.0

Rectangle {
    id: chatPage

    color: Nheko.colors.window

    SplitView {
        anchors.fill: parent

        Rectangle {
            SplitView.minimumWidth: Nheko.avatarSize + Nheko.paddingSmall * 2
            SplitView.preferredWidth: Nheko.avatarSize + Nheko.paddingSmall * 2
            SplitView.maximumWidth: Nheko.avatarSize + Nheko.paddingSmall * 2
            color: Nheko.theme.sidebarBackground
        }

        RoomList {
            //SplitView.maximumWidth: Nheko.avatarSize * 7 + Nheko.paddingSmall * 2

            SplitView.minimumWidth: Nheko.avatarSize * 5 + Nheko.paddingSmall * 2
            SplitView.preferredWidth: Nheko.avatarSize * 5 + Nheko.paddingSmall * 2
        }

        TimelineView {
            id: timeline

            SplitView.fillWidth: true
            SplitView.minimumWidth: 400
        }

        handle: Rectangle {
            implicitWidth: 2
            color: SplitHandle.pressed ? Nheko.colors.highlight : (SplitHandle.hovered ? Nheko.colors.light : Nheko.theme.separator)
        }

    }

    PrivacyScreen {
        anchors.fill: parent
        visible: Settings.privacyScreen
        screenTimeout: Settings.privacyScreenTimeout
        timelineRoot: timeline
    }

}
