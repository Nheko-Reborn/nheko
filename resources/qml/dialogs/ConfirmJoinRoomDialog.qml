// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../ui"
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0

ApplicationWindow {
    id: joinRoomRoot

    required property RoomSummary summary

    title: summary.isSpace ? qsTr("Confirm community join") : qsTr("Confirm room join")
    modality: Qt.WindowModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    palette: Nheko.colors
    color: Nheko.colors.window
    width: 350
    height: content.implicitHeight + Nheko.paddingLarge + footer.implicitHeight

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: dbb.rejected()
    }

    ColumnLayout {
        id: content
        spacing: Nheko.paddingMedium
        anchors.margins: Nheko.paddingMedium
        anchors.fill: parent

        Avatar {
            Layout.topMargin: Nheko.paddingMedium
            url: summary.roomAvatarUrl.replace("mxc://", "image://MxcImage/")
            roomid: summary.roomid
            displayName: summary.roomName
            height: 130
            width: 130
            Layout.alignment: Qt.AlignHCenter
        }

        Spinner {
            Layout.alignment: Qt.AlignHCenter
            visible: !summary.isLoaded
            foreground: Nheko.colors.mid
            running: !summary.isLoaded
        }

        TextEdit {
            readOnly: true
            textFormat: TextEdit.RichText
            text: summary.roomName
            font.pixelSize: fontMetrics.font.pixelSize * 2
            color: Nheko.colors.text

            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: TextEdit.AlignHCenter
            wrapMode: TextEdit.Wrap
            selectByMouse: true
        }
        TextEdit {
            readOnly: true
            textFormat: TextEdit.RichText
            text: summary.roomid
            font.pixelSize: fontMetrics.font.pixelSize * 0.8
            color: Nheko.colors.text

            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: TextEdit.AlignHCenter
            wrapMode: TextEdit.Wrap
            selectByMouse: true
        }
        RowLayout {
            spacing: Nheko.paddingMedium
            Layout.alignment: Qt.AlignHCenter

            MatrixText {
                text: qsTr("%n member(s)", "", summary.memberCount)
            }

            ImageButton {
                image: ":/icons/icons/ui/people.svg"
                enabled: false
            }

        }
        TextEdit {
            readOnly: true
            textFormat: TextEdit.RichText
            text: summary.roomTopic
            color: Nheko.colors.text

            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: TextEdit.AlignHCenter
            wrapMode: TextEdit.Wrap
            selectByMouse: true
        }

        Label {
            id: promptLabel

            text: summary.isKnockOnly ? qsTr("This room can't be joined directly. You can however knock on the room and room members can accept or decline this join request. You can additionally provide a reason for them to let you in below:") : qsTr("Do you want to join this room? You can optionally add a reason below:")
            color: Nheko.colors.text
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.bold: true
        }

        MatrixTextField {
            id: reason

            focus: true
            Layout.fillWidth: true
            text: joinRoomRoot.summary.reason
        }

    }

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Cancel
        onAccepted: {
            summary.reason = reason.text;
            summary.join();
            joinRoomRoot.close();
        }
        onRejected: {
            joinRoomRoot.close();
        }

        Button {
            text: summary.isKnockOnly ? qsTr("Knock") : qsTr("Join")
            enabled: input.text.match("#.+?:.{3,}")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }

    }

}
