// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQml.Models

Item {
    id: root

    property Component delegate
    property alias model: visualModel.model

    Component {
        id: dragDelegate

        MouseArea {
            id: dragArea

            property bool held: false
            required property int index
            required property var model

            drag.axis: Drag.YAxis
            drag.target: held ? content : undefined
            enabled: model.moveable == undefined || model.moveable
            height: content.height

            onHeldChanged: if (held)
                ListView.view.currentIndex = dragArea.index
            else
                ListView.view.currentIndex = -1
            onPressAndHold: held = true
            onPressed: if (mouse.source !== Qt.MouseEventNotSynthesized) {
                held = true;
            }
            onReleased: held = false

            anchors {
                left: parent.left
                right: parent.right
            }
            Rectangle {
                id: content

                Drag.active: dragArea.held
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2
                Drag.source: dragArea
                border.color: palette.highlight
                border.width: dragArea.enabled ? 1 : 0
                color: dragArea.held ? palette.highlight : palette.base
                height: actualDelegate.implicitHeight + 4
                radius: 2
                width: dragArea.width

                Behavior on color {
                    ColorAnimation {
                        duration: 100
                    }
                }
                states: State {
                    when: dragArea.held

                    ParentChange {
                        parent: root
                        target: content
                    }
                    AnchorChanges {
                        target: content

                        anchors {
                            horizontalCenter: undefined
                            verticalCenter: undefined
                        }
                    }
                }

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                Loader {
                    id: actualDelegate

                    property int index: dragArea.index
                    property var model: dragArea.model
                    property int offset: -view.contentY + dragArea.y

                    sourceComponent: root.delegate

                    anchors {
                        fill: parent
                        margins: 2
                    }
                }
            }
            DropArea {
                enabled: index != 0 || model.moveable == undefined || model.moveable

                onEntered: drag => {
                    visualModel.model.move(drag.source.index, dragArea.index);
                }

                anchors {
                    fill: parent
                    margins: 8
                }
            }
        }
    }
    DelegateModel {
        id: visualModel

        delegate: dragDelegate
    }
    ListView {
        id: view

        cacheBuffer: 50
        clip: true
        highlightRangeMode: ListView.ApplyRange
        model: visualModel
        preferredHighlightBegin: 0.2 * height
        preferredHighlightEnd: 0.8 * height
        spacing: 4

        anchors {
            fill: parent
            margins: 2
        }
    }
}
