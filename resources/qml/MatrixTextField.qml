// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import im.nheko 1.0


ColumnLayout {
    id: c
    property color backgroundColor: Nheko.colors.base
    property alias color: labelC.color
    property alias textPadding: input.padding
    property alias text: input.text
    property alias label: labelC.text
    property alias placeholderText: input.placeholderText
    property alias font: input.font
    property alias echoMode: input.echoMode
    property alias selectByMouse: input.selectByMouse

    Timer {
        id: timer
        interval: 350
        onTriggered: editingFinished()
    }

    onTextChanged: timer.restart()

    signal textEdited
    signal accepted
    signal editingFinished

    function forceActiveFocus() {
        input.forceActiveFocus();
    }

    function clear() {
        input.clear();
    }

    ToolTip.delay: Nheko.tooltipDelay
    ToolTip.visible: hover.hovered

    spacing: 0

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: labelC.contentHeight
        Layout.margins: input.padding
        Layout.bottomMargin: Nheko.paddingSmall
        visible: labelC.text

        z: 1

        Label {
            id: labelC

            y: contentHeight + input.padding + Nheko.paddingSmall
            enabled: false

            palette: Nheko.colors
            color: Nheko.colors.text
            font.pixelSize: input.font.pixelSize
            font.weight: Font.DemiBold
            font.letterSpacing: input.font.pixelSize * 0.02
            width: parent.width

            state: labelC.text && (input.activeFocus == true || input.text) ? "focused" : ""

            states: State {
                name: "focused"

                PropertyChanges {
                    target: labelC
                    y: 0
                }

                PropertyChanges {
                    target: input
                    opacity: 1
                }

            }

            transitions: Transition {
                from: ""
                to: "focused"
                reversible: true

                NumberAnimation {
                    target: labelC
                    properties: "y"
                    duration: 210
                    easing.type: Easing.InCubic
                    alwaysRunToEnd: true
                }

                NumberAnimation {
                    target: input
                    properties: "opacity"
                    duration: 210
                    easing.type: Easing.InCubic
                    alwaysRunToEnd: true
                }

            }
        }
    }

    TextField {
        id: input
        Layout.fillWidth: true

        palette: Nheko.colors
        color: labelC.color
        opacity: labelC.text ? 0 : 1
        focus: true

        onTextEdited: c.textEdited()
        onAccepted: c.accepted()
        onEditingFinished: c.editingFinished()

        background: Rectangle {
            id: backgroundRect

            color: labelC.text ? "transparent" : backgroundColor
        }

    }

    Rectangle {
        id: blueBar

        Layout.fillWidth: true

        color: Nheko.colors.highlight
        height: 1

        Rectangle {
            id: blackBar

            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            height: parent.height*2
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
                    duration: 310
                    easing.type: Easing.InCubic
                    alwaysRunToEnd: true
                }

            }

        }

    }

    HoverHandler {
        id: hover
        enabled: c.ToolTip.text
    }
}
