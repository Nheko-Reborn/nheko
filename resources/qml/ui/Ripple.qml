// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import im.nheko 1.0

Item {
    id: wrapper
    property color color

    Loader {
        Component {
            id: ripple
            RippleImpl { color: color; parent: wrapper.parent; }
        }
        sourceComponent: !Settings.reducedMotion ? ripple : undefined
    }
}
