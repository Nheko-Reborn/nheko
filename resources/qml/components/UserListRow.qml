// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import im.nheko 1.0

ItemDelegate {
    property string avatarUrl
    property alias bgColor: background.color
    property alias displayName: avatar.displayName
    property alias userid: avatar.userid

    implicitHeight: layout.implicitHeight + Nheko.paddingSmall * 2

    background: Rectangle {
        id: background

    }

    GridLayout {
        id: layout

        anchors.centerIn: parent
        columnSpacing: Nheko.paddingMedium
        columns: 2
        rowSpacing: Nheko.paddingSmall
        rows: 2
        width: parent.width - Nheko.paddingSmall * 2

        Avatar {
            id: avatar

            Layout.alignment: Qt.AlignLeft
            Layout.preferredHeight: Nheko.avatarSize
            Layout.preferredWidth: Nheko.avatarSize
            Layout.rowSpan: 2
            enabled: false
            url: avatarUrl.replace("mxc://", "image://MxcImage/")
        }
        Label {
            Layout.fillWidth: true
            color: TimelineManager.userColor(userid, palette.window)
            font.pointSize: fontMetrics.font.pointSize
            text: displayName
        }
        Label {
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            color: palette.buttonText
            font.pointSize: fontMetrics.font.pointSize * 0.9
            text: userid
        }
    }
}
