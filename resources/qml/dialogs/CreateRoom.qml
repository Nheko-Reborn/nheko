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

    property bool space: false

    title: space ? qsTr("New community") : qsTr("New Room")
    minimumWidth: Math.max(rootLayout.implicitWidth+2*rootLayout.anchors.margins, footer.implicitWidth + Nheko.paddingLarge)
    minimumHeight: rootLayout.implicitHeight+footer.implicitHeight+2*rootLayout.anchors.margins
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

    onVisibilityChanged: {
        newRoomName.forceActiveFocus();
    }

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: createRoomRoot.close()
    }
    GridLayout {
        id: rootLayout
        anchors.fill: parent
        anchors.margins: Nheko.paddingLarge
        columns: 2
        rowSpacing: Nheko.paddingMedium

        MatrixTextField {
            id: newRoomName
            Layout.columnSpan: 2
            Layout.fillWidth: true

            focus: true
            label: qsTr("Name")
            placeholderText: qsTr("No name")
        }
        MatrixTextField {
            id: newRoomTopic
            Layout.columnSpan: 2
            Layout.fillWidth: true

            focus: true
            label: qsTr("Topic")
            placeholderText: qsTr("No topic")
        }

        Item {
            Layout.preferredHeight: newRoomName.height / 2
        }

        RowLayout {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Label {
                Layout.preferredWidth: implicitWidth
                text: "#"
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
            text: qsTr("Public")
            color: Nheko.colors.text
            HoverHandler {
                id: privateHover
            }
            ToolTip.visible: privateHover.hovered
            ToolTip.text: qsTr("Public rooms can be joined by anyone, private rooms need explicit invites.")
            ToolTip.delay: Nheko.tooltipDelay
        }
        ToggleButton {
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: implicitWidth
            id: isPublic
            checked: false
        }
        Label {
            visible: !space
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
            visible: !space
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: implicitWidth
            id: isTrusted
            checked: false
            enabled: !isPublic.checked
        }
        Label {
            visible: !space
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
            visible: !space
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: implicitWidth
            id: isEncrypted
            checked: false
        }

        Item {Layout.fillHeight: true}
    }
    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Cancel
        Button {
            text: qsTr("Create Room")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        onRejected: createRoomRoot.close();
        onAccepted: {
            var preset = 0;

            if (isPublic.checked) {
                preset = 1;
            }
            else {
                preset = isTrusted.checked ? 2 : 0;
            }
            Nheko.createRoom(space, newRoomName.text, newRoomTopic.text, newRoomAlias.text, isEncrypted.checked, preset)
            createRoomRoot.close();
        }
    }
}
