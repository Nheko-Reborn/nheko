// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import im.nheko 1.0

ColumnLayout {
    id: c

    property color backgroundColor: palette.base
    property alias color: labelC.color
    property alias echoMode: input.echoMode
    property alias font: input.font
    property var hasClear: false
    property alias label: labelC.text
    property alias placeholderText: input.placeholderText
    property alias selectByMouse: input.selectByMouse
    property alias text: input.text
    property alias textPadding: input.padding

    signal accepted
    signal editingFinished
    signal textEdited

    function clear() {
        input.clear();
    }
    function forceActiveFocus() {
        input.forceActiveFocus();
    }

    ToolTip.delay: Nheko.tooltipDelay
    ToolTip.visible: hover.hovered
    spacing: 0

    onTextChanged: timer.restart()

    Timer {
        id: timer

        interval: 350

        onTriggered: editingFinished()
    }
    Item {
        Layout.bottomMargin: Nheko.paddingSmall
        Layout.fillWidth: true
        Layout.margins: input.padding
        Layout.preferredHeight: labelC.contentHeight
        visible: labelC.text
        z: 1

        Label {
            id: labelC

            color: palette.text
            enabled: false
            font.letterSpacing: input.font.pixelSize * 0.02
            font.pixelSize: input.font.pixelSize
            font.weight: Font.DemiBold
            state: labelC.text && (input.activeFocus == true || input.text) ? "focused" : ""
            width: parent.width
            y: contentHeight + input.padding + Nheko.paddingSmall

            states: State {
                name: "focused"

                PropertyChanges {
                    labelC.y: 0
                }
                PropertyChanges {
                    input.opacity: 1
                }
            }
            transitions: Transition {
                from: ""
                reversible: true
                to: "focused"

                NumberAnimation {
                    alwaysRunToEnd: true
                    duration: 210
                    easing.type: Easing.InCubic
                    properties: "y"
                    target: labelC
                }
                NumberAnimation {
                    alwaysRunToEnd: true
                    duration: 210
                    easing.type: Easing.InCubic
                    properties: "opacity"
                    target: input
                }
            }
        }
    }
    TextField {
        id: input

        Layout.fillWidth: true
        color: labelC.color
        focus: true
        opacity: labelC.text ? 0 : 1

        background: Rectangle {
            id: backgroundRect

            color: labelC.text ? "transparent" : backgroundColor
        }

        onAccepted: c.accepted()
        onEditingFinished: c.editingFinished()
        onTextEdited: c.textEdited()

        ImageButton {
            id: clearText

            focusPolicy: Qt.NoFocus
            hoverEnabled: true
            image: ":/icons/icons/ui/round-remove-button.svg"
            visible: c.hasClear && searchField.text !== ''

            onClicked: {
                searchField.clear();
                topBar.searchString = "";
            }

            anchors {
                bottom: parent.bottom
                right: parent.right
                rightMargin: Nheko.paddingSmall
                top: parent.top
            }
        }
    }
    Rectangle {
        id: blueBar

        Layout.fillWidth: true
        color: palette.highlight
        Layout.preferredHeight: 1

        Rectangle {
            id: blackBar

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            color: palette.text
            height: parent.height * 2
            width: 0

            states: State {
                name: "focused"
                when: input.activeFocus == true

                PropertyChanges {
                    blackBar.width: blueBar.width
                }
            }
            transitions: Transition {
                from: ""
                reversible: true
                to: "focused"

                NumberAnimation {
                    alwaysRunToEnd: true
                    duration: 310
                    easing.type: Easing.InCubic
                    properties: "width"
                    target: blackBar
                }
            }
        }
    }
    HoverHandler {
        id: hover

        enabled: c.ToolTip.text
    }
}
