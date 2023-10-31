// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

Item {
    id: ripple

    property color color: "#22000000"
    property real maxRadius: Math.max(width, height)
    readonly property real opacityAnimationDuration: 300
    readonly property real radiusAnimationRate: 0.05
    readonly property real radiusTailAnimationRate: 0.5
    property var rippleTarget: parent

    anchors.fill: parent

    PointHandler {
        id: ph

        target: Rectangle {
            id: backgroundLayer

            anchors.fill: parent
            clip: true
            color: "transparent"
            parent: rippleTarget

            Rectangle {
                id: circle

                property real centerX
                property real centerY

                color: ripple.color
                height: radius * 2
                radius: 0
                state: ph.active ? "ACTIVE" : "NORMAL"
                width: radius * 2
                x: centerX - radius
                y: centerY - radius

                states: [
                    State {
                        name: "NORMAL"
                    },
                    State {
                        name: "ACTIVE"
                    }
                ]
                transitions: [
                    Transition {
                        from: "NORMAL"
                        to: "ACTIVE"

                        SequentialAnimation {
                            //PropertyAction { target: circle; property: "centerX"; value: ph.point.position.x }
                            //PropertyAction { target: circle; property: "centerY"; value: ph.point.position.y }
                            PropertyAction {
                                property: "visible"
                                target: circle
                                value: true
                            }
                            PropertyAction {
                                property: "opacity"
                                target: circle
                                value: 1
                            }
                            NumberAnimation {
                                id: radius_animation

                                duration: ripple.maxRadius / ripple.radiusAnimationRate
                                from: 0
                                properties: "radius"
                                target: circle
                                to: ripple.maxRadius

                                easing {
                                    type: Easing.OutQuad
                                }
                            }
                        }
                    },
                    Transition {
                        from: "ACTIVE"
                        to: "NORMAL"

                        SequentialAnimation {
                            ParallelAnimation {
                                NumberAnimation {
                                    id: radius_tail_animation

                                    duration: ripple.maxRadius / ripple.radiusTailAnimationRate
                                    properties: "radius"
                                    target: circle
                                    to: ripple.maxRadius

                                    easing {
                                        type: Easing.Linear
                                    }
                                }
                                NumberAnimation {
                                    id: opacity_animation

                                    duration: ripple.opacityAnimationDuration
                                    properties: "opacity"
                                    target: circle
                                    to: 0

                                    easing {
                                        type: Easing.InQuad
                                    }
                                }
                            }
                            PropertyAction {
                                property: "visible"
                                target: circle
                                value: false
                            }
                        }
                    }
                ]
            }
        }

        onGrabChanged: (_, point) => {
            circle.centerX = point.position.x;
            circle.centerY = point.position.y;
        }
    }
}
