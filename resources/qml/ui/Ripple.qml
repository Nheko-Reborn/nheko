// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtGraphicalEffects 1.0
import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: ripple

    property color color: "#22000000"
    property real maxRadius: Math.max(width, height)
    readonly property real radiusAnimationRate: 0.05
    readonly property real radiusTailAnimationRate: 0.5
    readonly property real opacityAnimationDuration: 300
    property var rippleTarget: parent

    anchors.fill: parent

    PointHandler {
        id: ph

        onGrabChanged: {
            circle.centerX = point.position.x
            circle.centerY = point.position.y
        }

        target: Rectangle {
            id: backgroundLayer
            parent: rippleTarget

            anchors.fill: parent
            color: "transparent"
            clip: true

            Rectangle {
                id: circle

                property real centerX
                property real centerY

                x: centerX - radius
                y: centerY - radius

                height: radius*2
                width: radius*2
                radius: 0
                color: ripple.color

                state: ph.active ? "ACTIVE" : "NORMAL"
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
                            PropertyAction { target: circle; property: "visible"; value: true }
                            PropertyAction { target: circle; property: "opacity"; value: 1 }

                            NumberAnimation {
                                id: radius_animation

                                target: circle
                                properties: "radius"
                                from: 0
                                to: ripple.maxRadius
                                duration: ripple.maxRadius / ripple.radiusAnimationRate

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

                                    target: circle
                                    properties: "radius"
                                    to: ripple.maxRadius
                                    duration: ripple.maxRadius / ripple.radiusTailAnimationRate

                                    easing {
                                        type: Easing.Linear
                                    }

                                }

                                NumberAnimation {
                                    id: opacity_animation

                                    target: circle
                                    properties: "opacity"
                                    to: 0
                                    duration: ripple.opacityAnimationDuration

                                    easing {
                                        type: Easing.InQuad
                                    }

                                }

                            }
                            PropertyAction { target: circle; property: "visible"; value: false }
                        }
                    }
                ]
            }
        }
    }
}
