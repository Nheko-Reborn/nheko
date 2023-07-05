// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import im.nheko 1.0

MatrixText {
    required property string typeString

    text: qsTr("unimplemented event: ") + typeString
//    width: parent.width
    color: palette.inactive.text
}
