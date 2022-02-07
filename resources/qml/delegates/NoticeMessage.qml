// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.nheko 1.0


TextMessage {
    property bool isStateEvent
    font.italic: true
    color: Nheko.colors.buttonText
    font.pointSize: isStateEvent? 0.75*fontMetrics.font.pointSize : 1*fontMetrics.font.pointSize
}
