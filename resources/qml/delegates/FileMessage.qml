// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Layouts 1.2
import im.nheko 1.0

Item {
    required property string eventId
    required property string filename
    required property string filesize
    property int maxWidth

    height: row.height + 24
    width: maxWidth
    implicitWidth: maxWidth

    RowLayout {
        id: row

        anchors.centerIn: parent
        width: parent.width - 24
        spacing: 15

        Rectangle {
            id: button

            color: Nheko.colors.light
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

            CursorShape {
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
                color: Nheko.colors.text
            }

            Text {
                id: filesize_

                Layout.fillWidth: true
                text: filesize
                textFormat: Text.PlainText
                elide: Text.ElideRight
                color: Nheko.colors.text
            }

        }

    }

    Rectangle {
        color: Nheko.colors.alternateBase
        z: -1
        radius: 10
        height: row.height + 24
        width: 44 + 24 + 24 + Math.max(Math.min(filesize_.width, filesize_.implicitWidth), Math.min(filename_.width, filename_.implicitWidth))
    }

}
