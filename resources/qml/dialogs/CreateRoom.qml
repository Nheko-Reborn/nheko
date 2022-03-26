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
        MatrixTextField {
            id: newRoomAlias
            Layout.columnSpan: 2
            Layout.fillWidth: true

            focus: true
            placeholderText: qsTr("Alias")
        }
        Label {
            Layout.preferredWidth: implicitWidth
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Room Visibility")
            color: Nheko.colors.text
        }
        ComboBox {
            id: newRoomVisibility
            Layout.preferredWidth: implicitWidth
            Layout.alignment: Qt.AlignRight
            model: [qsTr("Private"), qsTr("Public")]
        }
        Label {
            Layout.preferredWidth: implicitWidth
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Room Preset")
            color: Nheko.colors.text
        }
        ComboBox {
            id: newRoomPreset
            Layout.preferredWidth: implicitWidth
            Layout.alignment: Qt.AlignRight
            model: [qsTr("Private Chat"), qsTr("Public Chat"), qsTr("Trusted Private Chat")]
        }
    }
    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Cancel
        Button {
            text: "Create Room"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        onRejected: createRoomRoot.close();
        //onAccepted: createRoom(newRoomName.text, newRoomTopic.text, newRoomAlias.text, newRoomVisibility.index, newRoomPreset.index)
    }
}
