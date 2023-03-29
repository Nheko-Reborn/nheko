// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Particles 2.15

Item {
    readonly property int maxLifespan: Math.max(confettiEmitter.lifeSpan, rainfallEmitter.lifeSpan)

    function pulseConfetti()
    {
        confettiEmitter.pulse(parent.height * 2)
    }

    function pulseRainfall()
    {
        rainfallEmitter.pulse(parent.height * 3.3)
    }

    ParticleSystem {
        id: particleSystem

        Component.onCompleted: pause();
        paused: !shouldEffectsRun
    }

    Emitter {
        id: confettiEmitter

        group: "confetti"
        width: parent.width * 3/4
        enabled: false
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height
        emitRate: Math.min(400 * Math.sqrt(parent.width * parent.height) / 870, 1000)
        lifeSpan: 15000
        system: particleSystem
        maximumEmitted: 500
        velocityFromMovement: 8
        size: 16
        sizeVariation: 4
        velocity: PointDirection {
            x: 0
            y: -Math.min(450 * parent.height / 700, 1000)
            xVariation: Math.min(4 * parent.width / 7, 450)
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
        anchors.fill: parent
        magnitude: 350
        angle: 90
    }

    Emitter {
        id: rainfallEmitter

        group: "rain"
        width: parent.width
        enabled: false
        anchors.horizontalCenter: parent.horizontalCenter
        y: -60
        emitRate: parent.width / 50
        lifeSpan: 10000
        system: particleSystem
        velocity: PointDirection {
            x: 0
            y: 300
            xVariation: 0
            yVariation: 75
        }

        ItemParticle {
            system: particleSystem
            groups: ["rain"]
            fade: false
            delegate: Rectangle {
                width: 2
                height: 30 + 30 * Math.random()
                radius: 2
                color: "#0099ff"
            }
        }
    }
}
