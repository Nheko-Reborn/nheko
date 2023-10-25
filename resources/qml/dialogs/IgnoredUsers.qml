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

    title: qsTr("Ignored users")
    flags: Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 650
    width: 420
    minimumHeight: 420
    color: palette.window

    ListView {
        id: view
        anchors.fill: parent
        spacing: Nheko.paddingMedium
        footerPositioning: ListView.OverlayFooter

        model: TimelineManager.ignoredUsers
        header: ColumnLayout {
            Text {
                Layout.fillWidth: true
                Layout.maximumWidth: view.width
                wrapMode: Text.Wrap
                color: palette.text
                text: qsTr("Ignoring a user hides their messages (they can still see yours!).")
            }

            Item { Layout.preferredHeight: Nheko.paddingLarge }
        }
        delegate: RowLayout {
            property var profile: TimelineManager.getGlobalUserProfile(modelData)

            width: view.width

            Avatar {
                enabled: false
                displayName: profile.displayName
                userid: profile.userid
                url: profile.avatarUrl.replace("mxc://", "image://MxcImage/")
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                elide: Text.ElideRight
                color: palette.text
                text: modelData
            }

            ImageButton {
                Layout.preferredHeight: 24
                Layout.preferredWidth: 24
                image: ":/icons/icons/ui/dismiss.svg"
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Stop Ignoring.")
                onClicked: profile.ignored = false
            }
        }
        footer: DialogButtonBox {
            z: 2
            width: view.width
            alignment: Qt.AlignRight
            standardButtons: DialogButtonBox.Ok
            onAccepted: ignoredUsers.close()

            background: Rectangle {
                anchors.fill: parent
                color: palette.window
            }
        }
    }
}
