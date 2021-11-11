// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import im.nheko 1.0

Slider {
    id: slider

    property real sliderWidth
    property real sliderHeight
    property bool alwaysShowSlider: true

    anchors.bottomMargin: orientation == Qt.Vertical ? Nheko.paddingMedium : undefined
    anchors.topMargin: orientation == Qt.Vertical ? Nheko.paddingMedium : undefined
    anchors.leftMargin: orientation == Qt.Vertical ? undefined : Nheko.paddingMedium
    anchors.rightMargin: orientation == Qt.Vertical ? undefined : Nheko.paddingMedium

    background: Rectangle {
        x: slider.leftPadding + (slider.orientation == Qt.Vertical ? slider.availableWidth / 2 - width / 2 : 0)
        y: slider.topPadding + (slider.orientation == Qt.Vertical ? 0 : slider.availableHeight / 2 - height / 2)
        // implicitWidth: slider.orientation == Qt.Vertical ? 8 : 100
        // implicitHeight: slider.orientation == Qt.Vertical ? 100 : 8
        width: slider.orientation == Qt.Vertical ? sliderWidth : slider.availableWidth
        height: slider.orientation == Qt.Vertical ? slider.availableHeight : sliderHeight
        radius: 2
        color: {
            if (slider.orientation == Qt.Vertical) {
                return Nheko.colors.highlight;
            } else {
                var col = Nheko.colors.buttonText;
                return Qt.rgba(col.r, col.g, col.b, 0.5);
            }
        }
        border.color: {
            var col = Nheko.colors.base;
            return Qt.rgba(col.r, col.g, col.b, 0.5);
        }

        Rectangle {
            width: slider.orientation == Qt.Vertical ? parent.width : slider.visualPosition * parent.width
            height: slider.orientation == Qt.Vertical ? slider.visualPosition * parent.height : parent.height
            color: {
                if (slider.orientation == Qt.Vertical) {
                    return Nheko.colors.buttonText;
                } else {
                    return Nheko.colors.highlight;
                }
            }
            radius: 2
        }

    }

    handle: Rectangle {
        x: {
            if (slider.orientation == Qt.Vertical)
                return slider.leftPadding + slider.availableWidth / 2 - width / 2;
            else
                return slider.leftPadding + slider.visualPosition * (slider.availableWidth - width);
        }
        y: {
            if (slider.orientation == Qt.Vertical)
                return slider.topPadding + slider.visualPosition * (slider.availableHeight - height);
            else
                return slider.topPadding + slider.availableHeight / 2 - height / 2;
        }
        implicitWidth: 16
        implicitHeight: 16
        radius: slider.width / 2
        color: Nheko.colors.highlight
        visible:  alwaysShowSlider || slider.hovered || slider.pressed || Settings.mobileMode
    }

}
