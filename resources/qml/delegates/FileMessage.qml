// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import im.nheko

Control {
    id: evRoot

    required property string eventId
    required property string filename
    required property string filesize

    padding: Settings.bubbles? 8 : 12
    //Layout.preferredHeight: rowa.implicitHeight + padding
    //Layout.maximumWidth: rowa.Layout.maximumWidth + metadataWidth + padding
    property int metadataWidth: 0
    property bool fitsMetadata: false

    Layout.maximumWidth: rowa.Layout.maximumWidth + padding * 2

    contentItem: RowLayout {
        id: rowa

        spacing: 16

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
                Layout.maximumWidth: implicitWidth + 1
                text: filename
                textFormat: Text.PlainText
                elide: Text.ElideRight
                color: palette.text
            }

            Text {
                id: filesize_

                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth + 1
                text: filesize
                textFormat: Text.PlainText
                elide: Text.ElideRight
                color: palette.text
            }

        }

    }

    background: Rectangle {
        color: palette.alternateBase
        radius: fontMetrics.lineSpacing / 2 + 2 * Nheko.paddingSmall
        visible: !Settings.bubbles // the bubble in a bubble looks odd
    }

}
