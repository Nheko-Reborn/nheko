// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import QtQuick.Window 2.15
import im.nheko
import "../components"

ColumnLayout {
    Item {
        Layout.fillHeight: true
    }
    Image {
        Layout.alignment: Qt.AlignHCenter
        height: 256
        source: "qrc:/logos/splash.png"
        width: 256
    }
    Label {
        Layout.alignment: Qt.AlignHCenter
        Layout.bottomMargin: 0
        Layout.fillWidth: true
        Layout.margins: Nheko.paddingLarge
        color: timelineRoot.palette.text
        font.pointSize: fontMetrics.font.pointSize * 2
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Welcome to nheko! The desktop client for the Matrix protocol.")
        wrapMode: Text.Wrap
    }
    Label {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true
        Layout.margins: Nheko.paddingLarge
        color: timelineRoot.palette.text
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
    Item {
        Layout.fillHeight: true
    }
}
