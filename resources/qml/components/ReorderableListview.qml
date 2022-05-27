// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQml.Models 2.1
import im.nheko 1.0
import ".."

Item {
    id: root

    property alias model: visualModel.model
    property Component delegate

    Component {
        id: dragDelegate

        MouseArea {
            id: dragArea

            required property var model
            required property int index

            enabled: model.moveable == undefined || model.moveable

            property bool held: false

            anchors { left: parent.left; right: parent.right }
            height: content.height

            drag.target: held ? content : undefined
            drag.axis: Drag.YAxis

            onPressAndHold: held = true
            onPressed: if (mouse.source !== Qt.MouseEventNotSynthesized) { held = true }
            onReleased: held = false
            onHeldChanged: if (held) ListView.view.currentIndex = dragArea.index; else ListView.view.currentIndex = -1

            Rectangle {
                id: content

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                width: dragArea.width; height: actualDelegate.implicitHeight + 4

                border.width: dragArea.enabled ? 1 : 0
                border.color: Nheko.colors.highlight

                color: dragArea.held ? Nheko.colors.highlight : Nheko.colors.base
                Behavior on color { ColorAnimation { duration: 100 } }

                radius: 2

                Drag.active: dragArea.held
                Drag.source: dragArea
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2

                states: State {
                    when: dragArea.held

                    ParentChange { target: content; parent: root }
                    AnchorChanges {
                        target: content
                        anchors { horizontalCenter: undefined; verticalCenter: undefined }
                    }
                }

                Loader {
                    id: actualDelegate
                    sourceComponent: root.delegate
                    property var model: dragArea.model
                    property int index: dragArea.index
                    property int offset: -view.contentY + dragArea.y
                    anchors { fill: parent; margins: 2 }
                }

            }

            DropArea {
                enabled: index != 0 || model.moveable == undefined || model.moveable
                anchors { fill: parent; margins: 8 }

                onEntered: (drag)=> {
                    visualModel.model.move(drag.source.index, dragArea.index)
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

            clip: true

            anchors { fill: parent; margins: 2 }
        ScrollHelper {
            flickable: parent
            anchors.fill: parent
        }

            model: visualModel

            highlightRangeMode: ListView.ApplyRange
            preferredHighlightBegin: 0.2 * height
            preferredHighlightEnd: 0.8 * height

            spacing: 4
            cacheBuffer: 50
        }


    }


