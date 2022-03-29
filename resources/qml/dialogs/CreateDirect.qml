// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.15
import QtQuick.Window 2.13
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQml.Models 2.15
import im.nheko 1.0

ApplicationWindow {
    id: createDirectRoot
    title: qsTr("Create Direct Chat")
    property var profile
    property bool otherUserHasE2ee: profile? profile.deviceList.rowCount() > 0 : true
    minimumHeight: layout.implicitHeight + footer.implicitHeight + Nheko.paddingLarge*2
    minimumWidth: Math.max(footer.implicitWidth, layout.implicitWidth)
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

    onVisibilityChanged: {
        userID.forceActiveFocus();
    }

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: createDirectRoot.close()
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Nheko.paddingLarge
        spacing: userID.height/4

        GridLayout {
            Layout.fillWidth: true
            rows: 2
            columns: 2
            rowSpacing: Nheko.paddingSmall
            columnSpacing: Nheko.paddingMedium

            Avatar {
                Layout.rowSpan: 2
                Layout.preferredWidth: Nheko.avatarSize
                Layout.preferredHeight: Nheko.avatarSize
                Layout.alignment: Qt.AlignLeft
                userid: profile? profile.userid : ""
                url: profile? profile.avatarUrl.replace("mxc://", "image://MxcImage/") : null
                displayName: profile? profile.displayName : ""
                enabled: false
            }
            Label {
                Layout.fillWidth: true
                text: profile? profile.displayName : ""
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

        MatrixTextField {
            id: userID
            property bool isValidMxid: text.match("@.+?:.{3,}")
            Layout.fillWidth: true
            focus: true
            label: qsTr("User to invite")
            placeholderText: qsTr("@user:server.tld")
            onTextChanged: {
                if(isValidMxid) {
                    profile = TimelineManager.getGlobalUserProfile(text);
                } else
                    profile = null;
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
                checked: otherUserHasE2ee
            }
        }

        Item {Layout.fillHeight: true}
    }
    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Cancel
        Button {
            text: "Start Direct Chat"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            enabled: userID.isValidMxid
        }
        onRejected: createDirectRoot.close();
        onAccepted: {
            profile.startChat(encryption.checked)
            createDirectRoot.close()
        }
    }
}
