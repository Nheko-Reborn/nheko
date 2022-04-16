// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import QtQuick 2.15
import QtQuick.Window 2.13
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQml.Models 2.15
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
            enabled: userID.isValidMxid
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
                color: TimelineManager.userColor(userID.text, timelineRoot.palette.window)
                font.pointSize: fontMetrics.font.pointSize
                text: profile ? profile.displayName : ""
            }
            Label {
                Layout.fillWidth: true
                color: timelineRoot.palette.placeholderText
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
                if (isValidMxid) {
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
                color: timelineRoot.palette.text
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
