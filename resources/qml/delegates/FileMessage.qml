// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Layouts 1.2
import im.nheko 1.0

Item {
    required property string eventId
    required property string filename
    required property string filesize

    height: rowa.height + (Settings.bubbles? 16: 24)
    implicitWidth: rowa.implicitWidth + metadataWidth
    property int metadataWidth
    property bool fitsMetadata: true

    RowLayout {
        id: rowa

        anchors.centerIn: parent
        width: parent.width - (Settings.bubbles? 16 : 24)
        spacing: 15

        Rectangle {
            id: button

            color: palette.light
            radius: 22
            height: 44
            width: 44

            Image {
                id: img

                height: 40
                width: 40
                sourceSize.height: 40
                sourceSize.width: 40

                anchors.centerIn: parent
                source: "qrc:/icons/icons/ui/download.svg"
                fillMode: Image.Pad
            }

            TapHandler {
                onSingleTapped: room.saveMedia(eventId)
                gesturePolicy: TapHandler.ReleaseWithinBounds
            }

            NhekoCursorShape {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
            }

        }

        ColumnLayout {
            id: col

            Text {
                id: filename_

                Layout.fillWidth: true
                text: filename
                textFormat: Text.PlainText
                elide: Text.ElideRight
                color: palette.text
            }

            Text {
                id: filesize_

                Layout.fillWidth: true
                text: filesize
                textFormat: Text.PlainText
                elide: Text.ElideRight
                color: palette.text
            }

        }

    }

    Rectangle {
        color: palette.alternateBase
        z: -1
        radius: 10
        anchors.fill: parent
        visible: !Settings.bubbles // the bubble in a bubble looks odd
    }

}
