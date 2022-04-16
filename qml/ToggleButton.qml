// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.5
import QtQuick 2.12
import QtQuick.Controls 2.12
import im.nheko

Switch {
    id: toggleButton
    implicitWidth: indicatorItem.width

    indicator: Item {
        id: indicatorItem
        implicitHeight: 24
        implicitWidth: 48
        y: parent.height / 2 - height / 2

        Rectangle {
            border.color: "#cccccc"
            color: toggleButton.checked ? "skyblue" : "grey"
            height: 3 * parent.height / 4
            radius: height / 2
            width: parent.width - height
            x: radius
            y: parent.height / 2 - height / 2
        }
        Rectangle {
            border.color: "#ebebeb"
            color: toggleButton.enabled ? "whitesmoke" : "#cccccc"
            height: width
            radius: width / 2
            width: parent.height
            x: toggleButton.checked ? parent.width - width : 0
            y: parent.height / 2 - height / 2
        }
    }
}
