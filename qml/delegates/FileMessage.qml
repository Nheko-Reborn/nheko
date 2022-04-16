// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.12
import QtQuick.Layouts 1.2
import im.nheko

Item {
    required property string eventId
    required property string filename
    required property string filesize
    property bool fitsMetadata: true
    property int metadataWidth

    height: row.height + (Settings.bubbles ? 16 : 24)
    implicitWidth: row.implicitWidth + metadataWidth
    width: parent.width

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 15
        width: parent.width - (Settings.bubbles ? 16 : 24)

        Rectangle {
            id: button
            color: timelineRoot.palette.light
            height: 44
            radius: 22
            width: 44

            Image {
                id: img
                anchors.centerIn: parent
                fillMode: Image.Pad
                height: 40
                source: "qrc:/icons/icons/ui/download.svg"
                sourceSize.height: 40
                sourceSize.width: 40
                width: 40
            }
            TapHandler {
                gesturePolicy: TapHandler.ReleaseWithinBounds

                onSingleTapped: room.saveMedia(eventId)
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
                color: timelineRoot.palette.text
                elide: Text.ElideRight
                text: filename
                textFormat: Text.PlainText
            }
            Text {
                id: filesize_
                Layout.fillWidth: true
                color: timelineRoot.palette.text
                elide: Text.ElideRight
                text: filesize
                textFormat: Text.PlainText
            }
        }
    }
    Rectangle {
        anchors.fill: parent
        color: timelineRoot.palette.alternateBase
        radius: 10
        visible: !Settings.bubbles // the bubble in a bubble looks odd
        z: -1
    }
}
