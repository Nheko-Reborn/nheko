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
    id: createDirectRoot
    title: qsTr("Create Direct Chat")
    property var profile: null
    minimumHeight: layout.implicitHeight+2*layout.anchors.margins+footer.height
    minimumWidth: footer.width
    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Nheko.paddingSmall
        MatrixTextField {
            id: userID
            Layout.fillWidth: true
            focus: true
            placeholderText: qsTr("Name")
            /*onTextChanged: {
                if(isValidMxid(text))
                    profile = getProfile(text);
                else
                    profile = null;
            }*/
        }

        GridLayout {
            Layout.fillWidth: true
            rows: 2
            columns: 2
            rowSpacing: Nheko.paddingSmall
            columnSpacing: Nheko.paddingMedium
            anchors.centerIn: parent

            Avatar {
                Layout.rowSpan: 2
                Layout.preferredWidth: Nheko.avatarSize
                Layout.preferredHeight: Nheko.avatarSize
                Layout.alignment: Qt.AlignLeft
                userid: profile.mxid
                url: profile.avatarUrl.replace("mxc://", "image://MxcImage/")
                displayName: profile.displayName
                enabled: false
            }
            Label {
                Layout.fillWidth: true
                text: "John Smith" //profile.displayName
                color: TimelineManager.userColor(userID.text, Nheko.colors.window)
                font.pointSize: fontMetrics.font.pointSize
            }

            Label {
                Layout.fillWidth: true
                text: userID.text
                color: Nheko.colors.buttonText
                font.pointSize: fontMetrics.font.pointSize * 0.9
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Encryption")
                color: Nheko.colors.text
            }
            ToggleButton {
                Layout.alignment: Qt.AlignRight
                id: encryption
                checked: true
            }
        }
    }
    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Cancel
        Button {
            text: "Start Direct Chat"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        onRejected: createDirectRoot.close();
        //onAccepted: createRoom(newRoomName.text, newRoomTopic.text, newRoomAlias.text, newRoomVisibility.index, newRoomPreset.index)
    }
}
