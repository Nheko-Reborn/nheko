// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import im.nheko

// FIXME(Nico): Don't use hardcoded colors.
Button {
    id: control

    implicitHeight: Math.ceil(control.contentItem.implicitHeight * 1.70)
    implicitWidth: Math.ceil(control.contentItem.implicitWidth + control.contentItem.implicitHeight)
    hoverEnabled: true

    property string iconImage: ""

    MultiEffect {
        anchors.fill: control.background
        shadowHorizontalOffset: 3
        shadowVerticalOffset: 3
        shadowBlur: 8
        shadowEnabled: true
        shadowColor: "#80000000"
        source: control.background
    }

    contentItem: RowLayout {
        spacing: 0
        anchors.centerIn: parent
        Image {
            Layout.leftMargin: Nheko.paddingMedium
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            Layout.preferredHeight: fontMetrics.font.pixelSize * 1.5
            Layout.preferredWidth:  fontMetrics.font.pixelSize * 1.5
            visible: !!iconImage
            source: iconImage
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: control.text
            //font: control.font
            font.capitalization: Font.AllUppercase
            font.pointSize: Math.ceil(fontMetrics.font.pointSize * 1.5)
            //font.capitalization: Font.AllUppercase
            color: palette.light
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    background: Rectangle {
        //height: control.contentItem.implicitHeight * 2
        //width: control.contentItem.implicitWidth * 2
        radius: height / 8
        color: Qt.lighter(palette.dark, control.down ? 1.4 : (control.hovered ? 1.2 : 1))
    }

}
