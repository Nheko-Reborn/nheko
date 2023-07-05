// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.5
import im.nheko 1.0


TextMessage {
    property bool isStateEvent
    font.italic: true
    color: palette.buttonText
    font.pointSize: isStateEvent? 0.8*Settings.fontSize : Settings.fontSize
    horizontalAlignment: isStateEvent? Text.AlignHCenter : undefined
}
