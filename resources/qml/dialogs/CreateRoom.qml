// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.15
import QtQuick.Window 2.13
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import im.nheko 1.0

ApplicationWindow {
    id: createRoomRoot
    title: qsTr("Create Room")
    minimumWidth: rootLayout.implicitWidth+2*rootLayout.anchors.margins
    minimumHeight: rootLayout.implicitHeight+footer.implicitHeight+2*rootLayout.anchors.margins
    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: createRoomRoot.close()
    }
    GridLayout {
        id: rootLayout
        anchors.fill: parent
        anchors.margins: Nheko.paddingSmall
        columns: 2
        MatrixTextField {
            id: newRoomName
            Layout.columnSpan: 2
            Layout.fillWidth: true

            focus: true
            placeholderText: qsTr("Name")
        }
        MatrixTextField {
            id: newRoomTopic
            Layout.columnSpan: 2
            Layout.fillWidth: true

            focus: true
            placeholderText: qsTr("Topic")
        }
        RowLayout {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Label {
                Layout.preferredWidth: implicitWidth
                text: qsTr("#")
                color: Nheko.colors.text
            }
            MatrixTextField {
                id: newRoomAlias
                focus: true
                placeholderText: qsTr("Alias")
            }
            Label {
                Layout.preferredWidth: implicitWidth
                property string userName: userInfoGrid.profile.userid
                text: userName.substring(userName.indexOf(":"))
                color: Nheko.colors.text
            }
        }
        Label {
            Layout.preferredWidth: implicitWidth
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Private")
            color: Nheko.colors.text
            HoverHandler {
                id: privateHover
            }
            ToolTip.visible: privateHover.hovered
            ToolTip.text: qsTr("Only invited users can join the room")
            ToolTip.delay: Nheko.tooltipDelay
        }
        ToggleButton {
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: implicitWidth
            id: isPrivate
            checked: true
        }
        Label {
            Layout.preferredWidth: implicitWidth
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Trusted")
            color: Nheko.colors.text
            HoverHandler {
                id: trustedHover
            }
            ToolTip.visible: trustedHover.hovered
            ToolTip.text: qsTr("All invitees are given the same power level as the creator")
            ToolTip.delay: Nheko.tooltipDelay
        }
        ToggleButton {
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: implicitWidth
            id: isTrusted
            checked: false
            enabled: isPrivate.checked
        }
        Label {
            Layout.preferredWidth: implicitWidth
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Encryption")
            color: Nheko.colors.text
            HoverHandler {
                id: encryptionHover
            }
            ToolTip.visible: encryptionHover.hovered
            ToolTip.text: qsTr("Caution: Encryption cannot be disabled")
            ToolTip.delay: Nheko.tooltipDelay
        }
        ToggleButton {
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: implicitWidth
            id: isEncrypted
            checked: false
        }
    }
    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Cancel
        Button {
            text: qsTr("Create Room")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        onRejected: createRoomRoot.close();
        //onAccepted: createRoom(newRoomName.text, newRoomTopic.text, newRoomAlias.text, newRoomVisibility.index, newRoomPreset.index)
    }
}
