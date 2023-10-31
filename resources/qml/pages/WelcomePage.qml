// SPDX-FileCopyrightText: Nheko Contributors
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
        Layout.preferredHeight: 256
        Layout.preferredWidth: 256
        source: "qrc:/logos/splash.png"
    }
    Label {
        Layout.alignment: Qt.AlignHCenter
        Layout.bottomMargin: 0
        Layout.fillWidth: true
        Layout.margins: Nheko.paddingLarge
        color: palette.text
        font.pointSize: fontMetrics.font.pointSize * 2
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Welcome to nheko! The desktop client for the Matrix protocol.")
        wrapMode: Text.Wrap
    }
    Label {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true
        Layout.margins: Nheko.paddingLarge
        color: palette.text
        font.pointSize: fontMetrics.font.pointSize * 1.5
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Enjoy your stay!")
        wrapMode: Text.Wrap
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        FlatButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: Nheko.paddingLarge
            text: qsTr("REGISTER")

            onClicked: {
                mainWindow.push(registerPage);
            }
        }
        FlatButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: Nheko.paddingLarge
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
            Layout.alignment: Qt.AlignRight
            Layout.margins: Nheko.paddingLarge
            checked: Settings.reducedMotion

            onCheckedChanged: Settings.reducedMotion = checked
        }
        Label {
            Layout.alignment: Qt.AlignLeft
            Layout.margins: Nheko.paddingLarge
            ToolTip.delay: Nheko.tooltipDelay
            ToolTip.text: qsTr("Nheko uses animations in several places to make stuff pretty. This allows you to turn those off if they make you feel unwell.")
            ToolTip.visible: hovered.hovered
            color: palette.text
            text: qsTr("Reduce animations")

            HoverHandler {
                id: hovered

            }
        }
    }
    Item {
        Layout.fillHeight: true
    }
}
