// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import QtQuick.Window 2.15
import im.nheko 1.0
import "../components/"

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
    Item {
        Layout.fillHeight: true
    }
}
