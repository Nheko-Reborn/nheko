// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigami as Kirigami

Kirigami.WheelHandler {
    id: wheelHandler
    target: parent
    filterMouseEvents: true
    keyNavigationEnabled: true
}
