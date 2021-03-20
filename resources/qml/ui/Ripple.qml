// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtGraphicalEffects 1.0
import QtQuick 2.10
import QtQuick.Controls 2.3

Item {
    id: ripple

    property alias clip: backgroundLayer.clip
    property real radius: 0
    property color color: "#22000000"
    property real maxRadius: Math.max(width, height)
    readonly property real radiusAnimationRate: 0.05
    readonly property real radiusTailAnimationRate: 0.5
    readonly property real opacityAnimationDuration: 300
    readonly property real diameter: radius * 2
    property real centerX
    property real centerY
    property var rippleTarget: parent

    function start() {
        console.log("Starting ripple animation");
        ripple.state = "ACTIVE";
    }

    function stop() {
        console.log("Stopping ripple animation");
        ripple.state = "NORMAL";
    }

    anchors.fill: parent
    state: "NORMAL"
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
                ScriptAction {
                    script: {
                        ripple.opacity = 1;
                        ripple.visible = true;
                    }
                }

                NumberAnimation {
                    id: radius_animation

                    target: ripple
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

                        target: ripple
                        properties: "radius"
                        to: ripple.maxRadius
                        duration: ripple.maxRadius / ripple.radiusTailAnimationRate

                        easing {
                            type: Easing.Linear
                        }

                    }

                    NumberAnimation {
                        id: opacity_animation

                        target: ripple
                        properties: "opacity"
                        to: 0
                        duration: ripple.opacityAnimationDuration

                        easing {
                            type: Easing.InQuad
                        }

                    }

                }

                ScriptAction {
                    script: {
                        ripple.visible = false;
                    }
                }

            }

        }
    ]

    Connections {
        // Button
        // Default to center

        function onPressed(mouse) {
            // MouseArea
            if (mouse) {
                ripple.centerX = mouse.x;
                ripple.centerY = mouse.y;
            } else if (rippleTarget.pressX) {
                ripple.centerX = rippleTarget.pressX;
                ripple.centerY = rippleTarget.pressY;
            } else {
                ripple.centerX = width / 2;
                ripple.centerY = height / 2;
            }
            ripple.start();
        }

        function onReleased() {
            ripple.stop();
        }

        function onExited() {
            ripple.stop();
        }

        function onCanceled() {
            ripple.stop();
        }

        function onClicked() {
            ripple.stop();
        }

        target: rippleTarget
        ignoreUnknownSignals: true
    }

    Rectangle {
        id: backgroundLayer

        anchors.fill: parent
        color: "transparent"
        clip: true

        Rectangle {
            id: circle

            x: ripple.centerX - ripple.radius
            y: ripple.centerY - ripple.radius
            height: ripple.diameter
            width: ripple.diameter
            radius: ripple.radius
            color: ripple.color
        }

    }

}
