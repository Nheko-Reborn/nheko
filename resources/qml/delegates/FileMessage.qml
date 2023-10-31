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
    property bool fitsMetadata: false
    //Layout.preferredHeight: rowa.implicitHeight + padding
    //Layout.maximumWidth: rowa.Layout.maximumWidth + metadataWidth + padding
    property int metadataWidth: 0

    Layout.maximumWidth: rowa.Layout.maximumWidth + padding * 2
    padding: Settings.bubbles ? 8 : 12

    background: Rectangle {
        color: palette.alternateBase
        radius: fontMetrics.lineSpacing / 2 + 2 * Nheko.paddingSmall
        visible: !Settings.bubbles // the bubble in a bubble looks odd
    }
    contentItem: RowLayout {
        id: rowa

        spacing: 16

        Rectangle {
            id: button

            Layout.preferredHeight: 44
            Layout.preferredWidth: 44
            color: palette.light
            radius: 22

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
                Layout.maximumWidth: implicitWidth + 1
                color: palette.text
                elide: Text.ElideRight
                text: evRoot.filename
                textFormat: Text.PlainText
            }
            Text {
                id: filesize_

                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth + 1
                color: palette.text
                elide: Text.ElideRight
                text: evRoot.filesize
                textFormat: Text.PlainText
            }
        }
    }
}
