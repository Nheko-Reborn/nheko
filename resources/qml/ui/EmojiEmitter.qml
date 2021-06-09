import QtQuick 2.0
import QtQuick.Particles 2.0

Item {
    id: bg

    readonly property int velocity: 50

    ParticleSystem {
        id: particleSys
    }

    Emitter {
        id: particles

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        width: parent.width
        system: particleSys
        emitRate: 10
        lifeSpan: (parent.height / bg.velocity) * 1000 //8000
        lifeSpanVariation: 1000
        maximumEmitted: 1000
        size: 5
        sizeVariation: 15

        velocity: AngleDirection {
            angle: 90
            angleVariation: 10
            magnitude: bg.velocity
        }

    }

    ItemParticle {
        id: particle

        system: particleSys
        delegate: itemDelegate
    }

    Component {
        id: itemDelegate

        Item {
            id: container

            x: bg.width / 2
            y: 0

            Text {
                anchors.fill: parent
                text: "🎉"
            }

        }

    }

}
