// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."

MatrixText {
    text: qsTr("unimplemented event: ") + model.data.typeString
    width: parent ? parent.width : undefined
    color: inactiveColors.text
}
