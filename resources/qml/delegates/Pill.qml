// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.5
import QtQuick.Controls 2.1
import im.nheko 1.0

Label {
    property bool isStateEvent
    color: Nheko.colors.text
    horizontalAlignment: Text.AlignHCenter
    height: Math.round(fontMetrics.height * 1.4)
    width: contentWidth * 1.2

    background: Rectangle {
        radius: parent.height / 2
        color: Nheko.colors.alternateBase
    }

}
