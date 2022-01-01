// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

Slider {
    id: control

    property color progressColor: Nheko.colors.highlight
    property bool alwaysShowSlider: true
    property int sliderRadius: 16

    value: 0
    implicitHeight: sliderRadius
    padding: 0

    background: Rectangle {
        x: control.leftPadding + handle.width / 2
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 200
        implicitHeight: control.sliderRadius / 4
        width: control.availableWidth - handle.width
        height: implicitHeight
        radius: height / 2
        color: Nheko.colors.buttonText

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            color: control.progressColor
            radius: 2
        }

    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * background.width
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: control.sliderRadius
        implicitHeight: control.sliderRadius
        radius: control.sliderRadius / 2
        color: control.progressColor
        visible: Settings.mobileMode || control.alwaysShowSlider || control.hovered || control.pressed
        border.color: control.progressColor
    }

}
