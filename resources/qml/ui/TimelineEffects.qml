// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Particles 2.15

Item {
    id: effectRoot

    readonly property int maxLifespan: Math.max(confettiEmitter.lifeSpan, rainfallEmitter.lifeSpan)
    required property bool shouldEffectsRun

    function pulseConfetti() {
        confettiEmitter.pulse(effectRoot.height * 2);
    }
    function pulseRainfall() {
        rainfallEmitter.pulse(effectRoot.height * 3.3);
    }
    function removeParticles() {
        particleSystem.reset();
    }

    visible: effectRoot.shouldEffectsRun

    ParticleSystem {
        id: particleSystem

        paused: !effectRoot.shouldEffectsRun
        running: effectRoot.shouldEffectsRun

        Component.onCompleted: stop()
    }
    Emitter {
        id: confettiEmitter

        anchors.horizontalCenter: effectRoot.horizontalCenter
        emitRate: Math.min(400 * Math.sqrt(effectRoot.width * effectRoot.height) / 870, 1000)
        enabled: false
        group: "confetti"
        lifeSpan: 15000
        maximumEmitted: 500
        size: 16
        sizeVariation: 4
        system: particleSystem
        velocityFromMovement: 8
        width: effectRoot.width * 3 / 4
        y: effectRoot.height

        velocity: PointDirection {
            x: 0
            xVariation: Math.min(4 * effectRoot.width / 7, 450)
            y: -Math.min(450 * effectRoot.height / 700, 1000)
            yVariation: 250
        }
    }
    ImageParticle {
        color: "white"
        colorVariation: 1
        entryEffect: ImageParticle.None
        groups: ["confetti"]
        rotationVelocity: 0
        rotationVelocityVariation: 360
        source: "qrc:/confettiparticle.svg"
        system: particleSystem

        xVector: PointDirection {
            x: 1
            xVariation: 0.2
            y: 0
            yVariation: 0.2
        }
        yVector: PointDirection {
            x: 0
            xVariation: 0.2
            y: 0.5
            yVariation: 0.2
        }
    }
    Gravity {
        anchors.fill: effectRoot
        angle: 90
        groups: ["confetti"]
        magnitude: 350
        system: particleSystem
    }
    Emitter {
        id: rainfallEmitter

        anchors.horizontalCenter: effectRoot.horizontalCenter
        emitRate: effectRoot.width / 30
        enabled: false
        group: "rain"
        lifeSpan: 10000
        system: particleSystem
        width: effectRoot.width
        y: -60

        velocity: PointDirection {
            x: 0
            xVariation: 0
            y: 400
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
            color: "#0099ff"
            colorVariation: 0
            entryEffect: ImageParticle.None
            groups: ["rain"]
            rotationVelocity: 0
            rotationVelocityVariation: 0
            source: "qrc:/confettiparticle.svg"
            system: particleSystem

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
