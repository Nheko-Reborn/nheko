// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

Slider {
    id: control

    property bool alwaysShowSlider: true
    property color progressColor: palette.highlight
    property int sliderRadius: 16

    implicitHeight: sliderRadius
    padding: 0
    value: 0

    background: Rectangle {
        color: palette.buttonText
        height: implicitHeight
        implicitHeight: control.sliderRadius / 4
        implicitWidth: 200
        radius: height / 2
        width: control.availableWidth - handle.width
        x: control.leftPadding + handle.width / 2
        y: control.topPadding + control.availableHeight / 2 - height / 2

        Rectangle {
            color: control.progressColor
            height: parent.height
            radius: 2
            width: control.visualPosition * parent.width
        }
    }
    handle: Rectangle {
        border.color: control.progressColor
        color: control.progressColor
        implicitHeight: control.sliderRadius
        implicitWidth: control.sliderRadius
        radius: control.sliderRadius / 2
        visible: Settings.mobileMode || control.alwaysShowSlider || control.hovered || control.pressed
        x: control.leftPadding + control.visualPosition * background.width
        y: control.topPadding + control.availableHeight / 2 - height / 2
    }
}
