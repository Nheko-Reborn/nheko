// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import im.nheko 1.0

MatrixText {
    text: qsTr("unimplemented event: ") + model.data.typeString
    width: parent ? parent.width : undefined
    color: Nheko.inactiveColors.text
}
