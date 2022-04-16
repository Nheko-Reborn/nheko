// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import QtQuick 2.15
import QtQuick.Window 2.13
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import im.nheko

ApplicationWindow {
    id: createRoomRoot
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    minimumHeight: rootLayout.implicitHeight + footer.implicitHeight + 2 * rootLayout.anchors.margins
    minimumWidth: Math.max(rootLayout.implicitWidth + 2 * rootLayout.anchors.margins, footer.implicitWidth + Nheko.paddingLarge)
    modality: Qt.NonModal
    title: qsTr("Create Room")

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Cancel

        onAccepted: {
            var preset = 0;
            if (isPublic.checked) {
                preset = 1;
            } else {
                preset = isTrusted.checked ? 2 : 0;
            }
            Nheko.createRoom(newRoomName.text, newRoomTopic.text, newRoomAlias.text, isEncrypted.checked, preset);
            createRoomRoot.close();
        }
        onRejected: createRoomRoot.close()

        Button {
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            text: qsTr("Create Room")
        }
    }

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
                color: timelineRoot.palette.text
                text: qsTr("#")
            }
            MatrixTextField {
                id: newRoomAlias
                focus: true
                placeholderText: qsTr("Alias")
            }
            Label {
                property string userName: userInfoGrid.profile.userid

                Layout.preferredWidth: implicitWidth
                color: timelineRoot.palette.text
                text: userName.substring(userName.indexOf(":"))
            }
        }
        Label {
            Layout.alignment: Qt.AlignLeft
            Layout.preferredWidth: implicitWidth
            ToolTip.delay: Nheko.tooltipDelay
            ToolTip.text: qsTr("Public rooms can be joined by anyone, private rooms need explicit invites.")
            ToolTip.visible: privateHover.hovered
            color: timelineRoot.palette.text
            text: qsTr("Public")

            HoverHandler {
                id: privateHover
            }
        }
        ToggleButton {
            id: isPublic
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: implicitWidth
            checked: false
        }
        Label {
            Layout.alignment: Qt.AlignLeft
            Layout.preferredWidth: implicitWidth
            ToolTip.delay: Nheko.tooltipDelay
            ToolTip.text: qsTr("All invitees are given the same power level as the creator")
            ToolTip.visible: trustedHover.hovered
            color: timelineRoot.palette.text
            text: qsTr("Trusted")

            HoverHandler {
                id: trustedHover
            }
        }
        ToggleButton {
            id: isTrusted
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: implicitWidth
            checked: false
            enabled: !isPublic.checked
        }
        Label {
            Layout.alignment: Qt.AlignLeft
            Layout.preferredWidth: implicitWidth
            ToolTip.delay: Nheko.tooltipDelay
            ToolTip.text: qsTr("Caution: Encryption cannot be disabled")
            ToolTip.visible: encryptionHover.hovered
            color: timelineRoot.palette.text
            text: qsTr("Encryption")

            HoverHandler {
                id: encryptionHover
            }
        }
        ToggleButton {
            id: isEncrypted
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: implicitWidth
            checked: false
        }
        Item {
            Layout.fillHeight: true
        }
    }
}
