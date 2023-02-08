// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import QtQuick.Window 2.15
import im.nheko 1.0
import "../components/"
import ".."

ColumnLayout {
    Item {
        Layout.fillHeight: true
    }

    Image {
        Layout.alignment: Qt.AlignHCenter
        source: "qrc:/logos/splash.png"
        height: 256
        width: 256
    }

    Label {
        Layout.margins: Nheko.paddingLarge
        Layout.bottomMargin: 0
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true
        text: qsTr("Welcome to nheko! The desktop client for the Matrix protocol.")
        color: Nheko.colors.text
        font.pointSize: fontMetrics.font.pointSize*2
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
    }
    Label {
        Layout.margins: Nheko.paddingLarge
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true
        text: qsTr("Enjoy your stay!")
        color: Nheko.colors.text
        font.pointSize: fontMetrics.font.pointSize*1.5
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
    }

    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        FlatButton {
            Layout.margins: Nheko.paddingLarge
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("REGISTER")
            onClicked: {
                mainWindow.push(registerPage);
            }
        }
        FlatButton {
            Layout.margins: Nheko.paddingLarge
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("LOGIN")
            onClicked: {
                mainWindow.push(loginPage);
            }
        }

        Item {
            Layout.fillWidth: true
        }

    }

    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.margins: Nheko.paddingLarge

        ToggleButton {
            Layout.margins: Nheko.paddingLarge
            Layout.alignment: Qt.AlignRight
            checked: Settings.reducedMotion
            onCheckedChanged: Settings.reducedMotion = checked
        }

        Label {
            Layout.alignment: Qt.AlignLeft
            Layout.margins: Nheko.paddingLarge
            text: qsTr("Reduce animations")
            color: Nheko.colors.text

            HoverHandler {
                id: hovered
            }
            ToolTip.visible: hovered.hovered
            ToolTip.text: qsTr("Nheko uses animations in several places to make stuff pretty. This allows you to turn those off if they make you feel unwell.")
            ToolTip.delay: Nheko.tooltipDelay
        }
    }
    Item {
        Layout.fillHeight: true
    }
}
