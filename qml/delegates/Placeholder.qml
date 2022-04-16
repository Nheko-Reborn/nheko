// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import im.nheko

MatrixText {
    required property string typeString

    //    width: parent.width
    color: timelineRoot.palette.inactive.text
    text: qsTr("unimplemented event: ") + typeString
}
