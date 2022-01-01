// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

BusyIndicator {
    id: control

    contentItem: Item {
        implicitWidth: 64
        implicitHeight: 64

        Item {
            id: item

            height: Math.min(parent.height, parent.width)
            width: height
            opacity: control.running ? 1 : 0

            RotationAnimator {
                target: item
                running: control.visible && control.running
                from: 0
                to: 360
                loops: Animation.Infinite
                duration: 2000
            }

            Repeater {
                id: repeater

                model: 6

                Rectangle {
                    implicitWidth: radius * 2
                    implicitHeight: radius * 2
                    radius: item.height / 6
                    color: Nheko.colors.text
                    opacity: (index + 2) / (repeater.count + 2)
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

            Behavior on opacity {
                OpacityAnimator {
                    duration: 250
                }

            }

        }

    }

}
