// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

TextMessage {
    font.italic: true
    color: colors.buttonText
    height: isReply ? Math.min(timelineRoot.height / 8, implicitHeight) : undefined
    clip: isReply
}
