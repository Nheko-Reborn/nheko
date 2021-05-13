// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

TextField {
    id: input

    palette: Nheko.colors

    Rectangle {
        id: blueBar

        anchors.top: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        color: Nheko.colors.highlight
        height: 1
        width: parent.width

        Rectangle {
            id: blackBar

            anchors.verticalCenter: blueBar.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            height: parent.height + 1
            width: 0
            color: Nheko.colors.text

            states: State {
                name: "focused"
                when: input.activeFocus == true

                PropertyChanges {
                    target: blackBar
                    width: blueBar.width
                }

            }

            transitions: Transition {
                from: ""
                to: "focused"
                reversible: true

                NumberAnimation {
                    target: blackBar
                    properties: "width"
                    duration: 500
                    easing.type: Easing.InOutQuad
                    alwaysRunToEnd: true
                }

            }

        }

    }

    background: Rectangle {
        color: Nheko.colors.base
    }

}
