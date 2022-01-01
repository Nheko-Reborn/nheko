// Copyright (C) 2016 Michael Bohlender, <michael.bohlender@kdemail.net>
// Copyright (C) 2017 Christian Mollekopf, <mollekopf@kolabsystems.com>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
* Shamelessly stolen from:
* https://cgit.kde.org/kube.git/tree/framework/qml/ScrollHelper.qml
*
* The MouseArea + interactive: false + maximumFlickVelocity are required
* to fix scrolling for desktop systems where we don't want flicking behaviour.
*
* See also:
* ScrollView.qml in qtquickcontrols
* qquickwheelarea.cpp in qtquickcontrols
*/

import QtQuick 2.9
import QtQuick.Controls 2.3

MouseArea {
    // console.warn("Delta: ", wheel.pixelDelta.y);
    // console.warn("Old position: ", flickable.contentY);
    // console.warn("New position: ", newPos);
    // breaks ListView's with headers...
    //if (typeof (flickableItem.headerItem) !== "undefined" && flickableItem.headerItem)
    //    minYExtent += flickableItem.headerItem.height;

    id: root

    property Flickable flickable
    property alias enabled: root.enabled

    function calculateNewPosition(flickableItem, wheel) {
        //Nothing to scroll
        if (flickableItem.contentHeight < flickableItem.height)
            return flickableItem.contentY;

        //Ignore 0 events (happens at least with Christians trackpad)
        if (wheel.pixelDelta.y == 0 && wheel.angleDelta.y == 0)
            return flickableItem.contentY;

        //pixelDelta seems to be the same as angleDelta/8
        var pixelDelta = 0;
        //The pixelDelta is a smaller number if both are provided, so pixelDelta can be 0 while angleDelta is still something. So we check the angleDelta
        if (wheel.angleDelta.y) {
            var wheelScrollLines = 3; //Default value of QApplication wheelScrollLines property
            var pixelPerLine = 20; //Default value in Qt, originally comes from QTextEdit
            var ticks = (wheel.angleDelta.y / 8) / 15; //Divide by 8 gives us pixels typically come in 15pixel steps.
            pixelDelta = ticks * pixelPerLine * wheelScrollLines;
        } else {
            pixelDelta = wheel.pixelDelta.y;
        }
        pixelDelta = Math.round(pixelDelta);
        if (!pixelDelta)
            return flickableItem.contentY;

        var minYExtent = flickableItem.originY + flickableItem.topMargin;
        var maxYExtent = (flickableItem.contentHeight + flickableItem.bottomMargin + flickableItem.originY) - flickableItem.height;
        //Avoid overscrolling
        return Math.max(minYExtent, Math.min(maxYExtent, flickableItem.contentY - pixelDelta));
    }

    propagateComposedEvents: true
    //Place the mouse area under the flickable
    z: -1
    onFlickableChanged: {
        if (enabled) {
            flickable.maximumFlickVelocity = 100000;
            flickable.boundsBehavior = Flickable.StopAtBounds;
            root.parent = flickable;
        }
    }
    acceptedButtons: Qt.NoButton
    onWheel: {
        var newPos = calculateNewPosition(flickable, wheel);
        // Show the scrollbars
        flickable.flick(0, 0);
        flickable.contentY = newPos;
        cancelFlickStateTimer.restart();
    }

    Timer {
        id: cancelFlickStateTimer

        //How long the scrollbar will remain visible
        interval: 500
        // Hide the scrollbars
        onTriggered: {
            flickable.cancelFlick();
            flickable.movementEnded();
        }
    }

}
