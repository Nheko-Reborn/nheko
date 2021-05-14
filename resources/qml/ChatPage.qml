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
            color: "blue"
        }

        Rectangle {
            SplitView.minimumWidth: Nheko.avatarSize * 3 + Nheko.paddingSmall * 2
            SplitView.preferredWidth: Nheko.avatarSize * 3 + Nheko.paddingSmall * 2
            SplitView.maximumWidth: Nheko.avatarSize * 7 + Nheko.paddingSmall * 2
            color: "red"
        }

        TimelineView {
            id: timeline

            SplitView.fillWidth: true
            SplitView.minimumWidth: 400
        }

    }

    PrivacyScreen {
        anchors.fill: parent
        visible: Settings.privacyScreen
        screenTimeout: Settings.privacyScreenTimeout
        timelineRoot: timeline
    }

}
