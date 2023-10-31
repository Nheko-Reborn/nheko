// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import im.nheko

ApplicationWindow {
    id: createDirectRoot

    property bool otherUserHasE2ee: profile ? profile.deviceList.rowCount() > 0 : true
    property var profile

    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    minimumHeight: layout.implicitHeight + footer.implicitHeight + Nheko.paddingLarge * 2
    minimumWidth: Math.max(footer.implicitWidth, layout.implicitWidth)
    modality: Qt.NonModal
    title: qsTr("Create Direct Chat")

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Cancel

        onAccepted: {
            profile.startChat(encryption.checked);
            createDirectRoot.close();
        }
        onRejected: createDirectRoot.close()

        Button {
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            enabled: userID.isValidMxid && profile
            text: "Start Direct Chat"
        }
    }

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
        spacing: userID.height / 4

        GridLayout {
            Layout.fillWidth: true
            columnSpacing: Nheko.paddingMedium
            columns: 2
            rowSpacing: Nheko.paddingSmall
            rows: 2

            Avatar {
                Layout.alignment: Qt.AlignLeft
                Layout.preferredHeight: Nheko.avatarSize
                Layout.preferredWidth: Nheko.avatarSize
                Layout.rowSpan: 2
                displayName: profile ? profile.displayName : ""
                enabled: false
                url: profile ? profile.avatarUrl.replace("mxc://", "image://MxcImage/") : null
                userid: profile ? profile.userid : ""
            }
            Label {
                Layout.fillWidth: true
                color: TimelineManager.userColor(userID.text, palette.window)
                font.pointSize: fontMetrics.font.pointSize
                text: profile ? profile.displayName : ""
            }
            Label {
                Layout.fillWidth: true
                color: palette.buttonText
                font.pointSize: fontMetrics.font.pointSize * 0.9
                text: userID.text
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
                // we can't use "isValidMxid" here, since the property might only be reevaluated after this change handler.
                if (text.match("@.+?:.{3,}")) {
                    profile = TimelineManager.getGlobalUserProfile(text);
                } else
                    profile = null;
            }
        }
        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: true
                color: palette.text
                text: qsTr("Encryption")
            }
            ToggleButton {
                id: encryption

                Layout.alignment: Qt.AlignRight
                checked: otherUserHasE2ee
            }
        }
        Item {
            Layout.fillHeight: true
        }
    }
}
