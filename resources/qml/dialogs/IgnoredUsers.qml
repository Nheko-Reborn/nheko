// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQml 2.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15
import QtQuick.Window 2.15
import im.nheko 1.0

Window {
    id: ignoredUsers
    required property list<string> users
    required property var profile

    title: qsTr("Ignored users")
    flags: Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 650
    width: 420
    minimumHeight: 420
    color: palette.window

    Connections {
        target: profile
        function onUnignoredUser(id, err) {
            if (err) {
                const text = qsTr("Failed to unignore \"%1\": %2").arg(id).arg(err)
                MainWindow.showNotification(text)
            } else {
                users = Array.from(users).filter(user => user !== id)
            }
        }
    }

    ListView {
        id: view
        anchors.fill: parent
        spacing: Nheko.paddingMedium

        model: users
        header: ColumnLayout {
            Text {
                Layout.fillWidth: true
                Layout.maximumWidth: view.width
                // Review request: Wrapping occurs with default width/font values, would it be better design to increase the window width?
                wrapMode: Text.Wrap
                color: palette.text
                text: qsTr("Ignoring a user hides their messages (they can still see yours!).")
            }

            // Review request: Is there a better way to do this?
            Item { Layout.preferredHeight: Nheko.paddingLarge }
        }
        delegate: RowLayout {
            width: view.width
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
                image: ":/icons/icons/ui/delete.svg"
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Stop Ignoring.")
                onClicked: profile.ignoredStatus(modelData, false)
            }
        }
    }
}