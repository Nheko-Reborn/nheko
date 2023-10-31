// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../ui"
import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0

ApplicationWindow {
    id: joinRoomRoot

    required property RoomSummary summary

    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: content.implicitHeight + Nheko.paddingLarge + footer.implicitHeight
    modality: Qt.WindowModal
    title: summary.isSpace ? qsTr("Confirm community join") : qsTr("Confirm room join")
    width: 350

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
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            enabled: input.text.match("#.+?:.{3,}")
            text: summary.isKnockOnly ? qsTr("Knock") : qsTr("Join")
        }
    }

    Shortcut {
        sequence: StandardKey.Cancel

        onActivated: dbb.rejected()
    }
    ColumnLayout {
        id: content

        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Avatar {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 130
            Layout.preferredWidth: 130
            Layout.topMargin: Nheko.paddingMedium
            displayName: summary.roomName
            roomid: summary.roomid
            url: summary.roomAvatarUrl.replace("mxc://", "image://MxcImage/")
        }
        Spinner {
            Layout.alignment: Qt.AlignHCenter
            foreground: palette.mid
            running: !summary.isLoaded
            visible: !summary.isLoaded
        }
        TextEdit {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            color: palette.text
            font.pixelSize: fontMetrics.font.pixelSize * 2
            horizontalAlignment: TextEdit.AlignHCenter
            readOnly: true
            selectByMouse: true
            text: summary.roomName
            textFormat: TextEdit.RichText
            wrapMode: TextEdit.Wrap
        }
        TextEdit {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            color: palette.text
            font.pixelSize: fontMetrics.font.pixelSize * 0.8
            horizontalAlignment: TextEdit.AlignHCenter
            readOnly: true
            selectByMouse: true
            text: summary.roomid
            textFormat: TextEdit.RichText
            wrapMode: TextEdit.Wrap
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Nheko.paddingMedium

            MatrixText {
                text: qsTr("%n member(s)", "", summary.memberCount)
            }
            ImageButton {
                enabled: false
                image: ":/icons/icons/ui/people.svg"
            }
        }
        TextEdit {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            color: palette.text
            horizontalAlignment: TextEdit.AlignHCenter
            readOnly: true
            selectByMouse: true
            text: summary.roomTopic
            textFormat: TextEdit.RichText
            wrapMode: TextEdit.Wrap
        }
        Label {
            id: promptLabel

            Layout.fillWidth: true
            color: palette.text
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            text: summary.isKnockOnly ? qsTr("This room can't be joined directly. You can, however, knock on the room and room members can accept or decline this join request. You can additionally provide a reason for them to let you in below:") : qsTr("Do you want to join this room? You can optionally add a reason below:")
            wrapMode: Text.Wrap
        }
        MatrixTextField {
            id: reason

            Layout.fillWidth: true
            focus: true
            text: joinRoomRoot.summary.reason
        }
    }
}
