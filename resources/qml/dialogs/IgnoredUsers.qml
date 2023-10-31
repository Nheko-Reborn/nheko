// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import im.nheko
import "../"

Window {
    id: ignoredUsers

    color: palette.window
    flags: Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 650
    minimumHeight: 420
    title: qsTr("Ignored users")
    width: 420

    ListView {
        id: view

        anchors.fill: parent
        footerPositioning: ListView.OverlayFooter
        model: TimelineManager.ignoredUsers
        spacing: Nheko.paddingMedium

        delegate: RowLayout {
            property var profile: TimelineManager.getGlobalUserProfile(modelData)

            width: view.width

            Avatar {
                displayName: profile.displayName
                enabled: false
                url: profile.avatarUrl.replace("mxc://", "image://MxcImage/")
                userid: profile.userid
            }
            Text {
                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: true
                color: palette.text
                elide: Text.ElideRight
                text: modelData
            }
            ImageButton {
                Layout.preferredHeight: 24
                Layout.preferredWidth: 24
                ToolTip.text: qsTr("Stop Ignoring.")
                ToolTip.visible: hovered
                hoverEnabled: true
                image: ":/icons/icons/ui/dismiss.svg"

                onClicked: profile.ignored = false
            }
        }
        footer: DialogButtonBox {
            alignment: Qt.AlignRight
            standardButtons: DialogButtonBox.Ok
            width: view.width
            z: 2

            background: Rectangle {
                anchors.fill: parent
                color: palette.window
            }

            onAccepted: ignoredUsers.close()
        }
        header: ColumnLayout {
            Text {
                Layout.fillWidth: true
                Layout.maximumWidth: view.width
                color: palette.text
                text: qsTr("Ignoring a user hides their messages (they can still see yours!).")
                wrapMode: Text.Wrap
            }
            Item {
                Layout.preferredHeight: Nheko.paddingLarge
            }
        }
    }
}
