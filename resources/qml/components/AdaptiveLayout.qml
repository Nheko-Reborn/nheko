// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import im.nheko

Container {
    //Component.onCompleted: {
    //    parent.width = Qt.binding(function() { return calculatedWidth; })
    //}

    id: container

    property Component handle: Rectangle {
        anchors.right: parent.right
        color: Nheko.theme.separator
        height: container.height
        width: visible ? 1 : 0
        z: 3
    }
    property Component handleToucharea: Item {
        id: splitter

        property int calculatedWidth: {
            if (!visible)
                return 0;
            else if (container.singlePageMode)
                return container.width;
            else
                return (collapsible && x < minimumWidth) ? collapsedWidth : x;
        }
        property int collapsedWidth: parent.collapsedWidth
        property bool collapsible: parent.collapsible
        property int maximumWidth: parent.maximumWidth
        property int minimumWidth: parent.minimumWidth

        enabled: !container.singlePageMode
        height: container.height
        width: 1
        x: parent.preferredWidth
        z: 3

        NhekoCursorShape {
            cursorShape: Qt.SizeHorCursor
            height: parent.height
            width: container.splitterGrabMargin * 2
            x: -container.splitterGrabMargin
        }
        DragHandler {
            id: dragHandler

            enabled: !container.singlePageMode
            grabPermissions: PointerHandler.CanTakeOverFromAnything | PointerHandler.ApprovesTakeOverByHandlersOfSameType
            margin: container.splitterGrabMargin
            xAxis.enabled: true
            xAxis.maximum: splitter.maximumWidth
            xAxis.minimum: splitter.minimumWidth - 1
            yAxis.enabled: false

            onActiveChanged: {
                if (!active) {
                    splitter.x = splitter.calculatedWidth;
                    splitter.parent.preferredWidth = splitter.calculatedWidth;
                }
            }
        }
        HoverHandler {
            enabled: !container.singlePageMode
            margin: container.splitterGrabMargin
        }
    }
    property alias pageIndex: view.currentIndex
    property bool singlePageMode: width < 800
    property int splitterGrabMargin: Nheko.paddingSmall

    contentItem: ListView {
        id: view

        boundsBehavior: Flickable.StopAtBounds
        currentIndex: container.singlePageMode ? container.pageIndex : 0
        highlightMoveDuration: (container.singlePageMode && !Settings.reducedMotion) ? 200 : 0
        highlightRangeMode: ListView.StrictlyEnforceRange
        interactive: singlePageMode
        model: container.contentModel
        orientation: ListView.Horizontal
        snapMode: ListView.SnapOneItem
    }

    Component.onCompleted: {
        for (var i = 0; i < count - 1; i++) {
            let handle_ = handle.createObject(contentChildren[i]);
            let split_ = handleToucharea.createObject(contentChildren[i]);
            contentChildren[i].width = Qt.binding(function () {
                    return split_.calculatedWidth;
                });
            contentChildren[i].splitterWidth = Qt.binding(function () {
                    return handle_.width;
                });
        }
        contentChildren[count - 1].width = Qt.binding(function () {
                if (container.singlePageMode) {
                    return container.width;
                } else {
                    var w = container.width;
                    for (var i = 0; i < count - 1; i++) {
                        if (contentChildren[i].width)
                            w = w - contentChildren[i].width;
                    }
                    return w;
                }
            });
        contentChildren[count - 1].splitterWidth = 0;
        for (var i = 0; i < count; i++) {
            contentChildren[i].height = Qt.binding(function () {
                    return container.height;
                });
            contentChildren[i].children[0].height = Qt.binding(function () {
                    return container.height;
                });
        }
    }
    onSinglePageModeChanged: if (!singlePageMode)
        pageIndex = 0
}
