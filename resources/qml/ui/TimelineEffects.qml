// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Particles 2.15

Item {
    id: effectRoot
    readonly property int maxLifespan: Math.max(confettiEmitter.lifeSpan, rainfallEmitter.lifeSpan)
    required property bool shouldEffectsRun
    visible: effectRoot.shouldEffectsRun

    function pulseConfetti()
    {
        confettiEmitter.pulse(effectRoot.height * 2)
    }

    function pulseRainfall()
    {
        rainfallEmitter.pulse(effectRoot.height * 3.3)
    }

    function removeParticles()
    {
        particleSystem.reset()
    }

    ParticleSystem {
        id: particleSystem

        Component.onCompleted: stop();
        paused: !effectRoot.shouldEffectsRun
        running: effectRoot.shouldEffectsRun
    }

    Emitter {
        id: confettiEmitter

        group: "confetti"
        width: effectRoot.width * 3/4
        enabled: false
        anchors.horizontalCenter: effectRoot.horizontalCenter
        y: effectRoot.height
        emitRate: Math.min(400 * Math.sqrt(effectRoot.width * effectRoot.height) / 870, 1000)
        lifeSpan: 15000
        system: particleSystem
        maximumEmitted: 500
        velocityFromMovement: 8
        size: 16
        sizeVariation: 4
        velocity: PointDirection {
            x: 0
            y: -Math.min(450 * effectRoot.height / 700, 1000)
            xVariation: Math.min(4 * effectRoot.width / 7, 450)
            yVariation: 250
        }
    }

    ImageParticle {
        system: particleSystem
        groups: ["confetti"]
        source: "qrc:/confettiparticle.svg"
        rotationVelocity: 0
        rotationVelocityVariation: 360
        colorVariation: 1
        color: "white"
        entryEffect: ImageParticle.None
        xVector: PointDirection {
            x: 1
            y: 0
            xVariation: 0.2
            yVariation: 0.2
        }
        yVector: PointDirection {
            x: 0
            y: 0.5
            xVariation: 0.2
            yVariation: 0.2
        }
    }

    Gravity {
        system: particleSystem
        groups: ["confetti"]
        anchors.fill: effectRoot
        magnitude: 350
        angle: 90
    }

    Emitter {
        id: rainfallEmitter

        group: "rain"
        width: effectRoot.width
        enabled: false
        anchors.horizontalCenter: effectRoot.horizontalCenter
        y: -60
        emitRate: effectRoot.width / 30
        lifeSpan: 10000
        system: particleSystem
        velocity: PointDirection {
            x: 0
            y: 400
            xVariation: 0
            yVariation: 75
        }

        // causes high CPU load, see: https://bugreports.qt.io/browse/QTBUG-117923
        //ItemParticle {
            //    system: particleSystem
            //    groups: ["rain"]
            //    fade: false
            //    visible: effectRoot.shouldEffectsRun
            //    delegate: Rectangle {
            //        width: 2
            //        height: 30 + 30 * Math.random()
            //        radius: 2
            //        color: "#0099ff"
            //    }
            //}

            ImageParticle {
                system: particleSystem
                groups: ["rain"]
                source: "qrc:/confettiparticle.svg"
                rotationVelocity: 0
                rotationVelocityVariation: 0
                colorVariation: 0
                color: "#0099ff"
                entryEffect: ImageParticle.None
                xVector: PointDirection {
                    x: 0.01
                    y: 0
                }
                yVector: PointDirection {
                    x: 0
                    y: 5
                }
            }
        }
    }
