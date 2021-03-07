// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Layouts 1.2
import im.nheko 1.0

Item {
    height: row.height + 24
    width: parent ? parent.width : undefined

    RowLayout {
        id: row

        anchors.centerIn: parent
        width: parent.width - 24
        spacing: 15

        Rectangle {
            id: button

            color: colors.light
            radius: 22
            height: 44
            width: 44

            Image {
                id: img

                anchors.centerIn: parent
                source: "qrc:/icons/icons/ui/arrow-pointing-down.png"
                fillMode: Image.Pad
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
            }

            TapHandler {
                onSingleTapped: TimelineManager.timeline.saveMedia(model.data.id)
            }

            CursorShape {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
            }

        }

        ColumnLayout {
            id: col

            Text {
                id: filename

                Layout.fillWidth: true
                text: model.data.filename
                textFormat: Text.PlainText
                elide: Text.ElideRight
                color: colors.text
            }

            Text {
                id: filesize

                Layout.fillWidth: true
                text: model.data.filesize
                textFormat: Text.PlainText
                elide: Text.ElideRight
                color: colors.text
            }

        }

    }

    Rectangle {
        color: colors.alternateBase
        z: -1
        radius: 10
        height: row.height + 24
        width: 44 + 24 + 24 + Math.max(Math.min(filesize.width, filesize.implicitWidth), Math.min(filename.width, filename.implicitWidth))
    }

}
