// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko

BusyIndicator {
    id: control
    contentItem: Item {
        implicitHeight: 64
        implicitWidth: 64

        Item {
            id: item
            height: Math.min(parent.height, parent.width)
            opacity: control.running ? 1 : 0
            width: height

            Behavior on opacity  {
                OpacityAnimator {
                    duration: 250
                }
            }

            RotationAnimator {
                duration: 2000
                from: 0
                loops: Animation.Infinite
                running: control.visible && control.running
                target: item
                to: 360
            }
            Repeater {
                id: repeater
                model: 6

                Rectangle {
                    color: timelineRoot.palette.text
                    implicitHeight: radius * 2
                    implicitWidth: radius * 2
                    opacity: (index + 2) / (repeater.count + 2)
                    radius: item.height / 6

                    transform: [
                        Translate {
                            y: -Math.min(item.width, item.height) * 0.5 + item.height / 6
                        },
                        Rotation {
                            angle: index / repeater.count * 360
                            origin.x: item.height / 2
                            origin.y: item.height / 2
                        }
                    ]
                }
            }
        }
    }
}
