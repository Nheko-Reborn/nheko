// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.5
import im.nheko


TextMessage {
    property bool isStateEvent
    font.italic: true
    color: timelineRoot.palette.placeholderText
    font.pointSize: isStateEvent? 0.8*Settings.fontSize : Settings.fontSize
    horizontalAlignment: isStateEvent? Text.AlignHCenter : undefined
}
