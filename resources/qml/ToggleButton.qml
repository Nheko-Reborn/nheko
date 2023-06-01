// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.5
import QtQuick 2.12
import QtQuick.Controls 2.12
import im.nheko 1.0

Switch {
    id: toggleButton

    implicitWidth: indicatorItem.width
    state: checked ? "on" : "off"

    indicator: Item {
        id: indicatorItem

        implicitHeight: 24
        implicitWidth: 48
        y: parent.height / 2 - height / 2

        Rectangle {
            id: track

            color: Qt.rgba(border.color.r, border.color.g, border.color.b, 0.6)
            height: parent.height * 0.6
            radius: height / 2
            width: parent.width - height
            x: radius
            y: parent.height / 2 - height / 2
        }
        Rectangle {
            id: handle

            border.color: "#767676"
            color: palette.button
            height: width
            radius: width / 2
            width: parent.height * 0.9
            y: parent.height / 2 - height / 2
        }
    }
    states: [
        State {
            name: "off"

            PropertyChanges {
                border.color: "#767676"
                target: track
            }
            PropertyChanges {
                target: handle
                x: 0
            }
        },
        State {
            name: "on"

            PropertyChanges {
                border.color: palette.highlight
                target: track
            }
            PropertyChanges {
                target: handle
                x: indicatorItem.width - handle.width
            }
        }
    ]
    transitions: [
        Transition {
            reversible: true
            to: "off"

            ParallelAnimation {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.InOutQuad
                    property: "x"
                    target: handle
                }
                ColorAnimation {
                    duration: 200
                    properties: "color,border.color"
                    target: track
                }
            }
        }
    ]
}
