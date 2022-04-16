// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

//import QtGraphicalEffects 1.12
import QtQuick 2.9
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.2
import im.nheko

// FIXME(Nico): Don't use hardcoded colors.
Button {
    id: control

    property string iconImage: ""

    hoverEnabled: true
    implicitHeight: Math.ceil(control.contentItem.implicitHeight * 1.70)
    implicitWidth: Math.ceil(control.contentItem.implicitWidth + control.contentItem.implicitHeight)

    background: Rectangle {
        color: Qt.lighter(timelineRoot.palette.dark, control.down ? 1.4 : (control.hovered ? 1.2 : 1))
        //height: control.contentItem.implicitHeight * 2
        //width: control.contentItem.implicitWidth * 2
        radius: height / 8
    }

    //DropShadow {
    //    anchors.fill: control.background
    //    horizontalOffset: 3
    //    verticalOffset: 3
    //    radius: 8
    //    samples: 17
    //    cached: true
    //    color: "#80000000"
    //    source: control.background
    //}
    contentItem: RowLayout {
        anchors.centerIn: parent
        spacing: 0

        Image {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            Layout.leftMargin: Nheko.paddingMedium
            Layout.preferredHeight: fontMetrics.font.pixelSize * 1.5
            Layout.preferredWidth: fontMetrics.font.pixelSize * 1.5
            source: iconImage
            visible: !!iconImage
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            //font.capitalization: Font.AllUppercase
            color: timelineRoot.palette.light
            elide: Text.ElideRight
            //font: control.font
            font.capitalization: Font.AllUppercase
            font.pointSize: Math.ceil(fontMetrics.font.pointSize * 1.5)
            horizontalAlignment: Text.AlignHCenter
            text: control.text
            verticalAlignment: Text.AlignVCenter
        }
    }
}
