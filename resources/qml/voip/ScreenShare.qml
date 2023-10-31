// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

Popup {
    anchors.centerIn: parent
    modal: true

    background: Rectangle {
        border.color: palette.windowText
        color: palette.window
    }

    Component.onCompleted: {
        frameRateCombo.currentIndex = frameRateCombo.find(Settings.screenShareFrameRate);
    }
    Component.onDestruction: {
        CallManager.closeScreenShare();
    }

    ColumnLayout {
        Label {
            Layout.alignment: Qt.AlignLeft
            Layout.bottomMargin: 16
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.topMargin: 16
            color: palette.windowText
            text: qsTr("Share desktop with %1?").arg(room.roomName)
        }
        RowLayout {
            Layout.bottomMargin: 8
            Layout.leftMargin: 8
            Layout.rightMargin: 8

            Label {
                Layout.alignment: Qt.AlignLeft
                color: palette.windowText
                text: qsTr("Method:")
            }
            ComboBox {
                id: screenshareType

                Layout.fillWidth: true
                model: CallManager.screenShareTypeList()

                onCurrentIndexChanged: CallManager.setScreenShareType(currentIndex)
            }
        }
        RowLayout {
            Layout.bottomMargin: 8
            Layout.leftMargin: 8
            Layout.rightMargin: 8

            Label {
                Layout.alignment: Qt.AlignLeft
                color: palette.windowText
                text: qsTr("Window:")
            }
            ComboBox {
                id: windowCombo

                Layout.fillWidth: true
                model: CallManager.windowList()
                visible: CallManager.screenShareType == Voip.X11
            }
            Button {
                highlighted: !CallManager.screenShareReady
                text: qsTr("Request screencast")
                visible: CallManager.screenShareType == Voip.XDP

                onClicked: {
                    Settings.screenShareHideCursor = hideCursorCheckBox.checked;
                    CallManager.setupScreenShareXDP();
                }
            }
        }
        RowLayout {
            Layout.bottomMargin: 8
            Layout.leftMargin: 8
            Layout.rightMargin: 8

            Label {
                Layout.alignment: Qt.AlignLeft
                color: palette.windowText
                text: qsTr("Frame rate:")
            }
            ComboBox {
                id: frameRateCombo

                Layout.fillWidth: true
                model: ["25", "20", "15", "10", "5", "2", "1"]
            }
        }
        GridLayout {
            Layout.margins: 8
            columns: 2
            rowSpacing: 10

            MatrixText {
                text: qsTr("Include your camera picture-in-picture")
            }
            ToggleButton {
                id: pipCheckBox

                Layout.alignment: Qt.AlignRight
                checked: CallManager.cameras.length > 0 && Settings.screenSharePiP
                enabled: CallManager.cameras.length > 0
            }
            MatrixText {
                ToolTip.text: qsTr("View your callee's camera like a regular video call")
                ToolTip.visible: hovered
                text: qsTr("Request remote camera")
            }
            ToggleButton {
                id: remoteVideoCheckBox

                Layout.alignment: Qt.AlignRight
                ToolTip.text: qsTr("View your callee's camera like a regular video call")
                ToolTip.visible: hovered
                checked: Settings.screenShareRemoteVideo
            }
            MatrixText {
                text: qsTr("Hide mouse cursor")
            }
            ToggleButton {
                id: hideCursorCheckBox

                Layout.alignment: Qt.AlignRight
                checked: Settings.screenShareHideCursor
            }
        }
        RowLayout {
            Layout.margins: 8

            Item {
                Layout.fillWidth: true
            }
            Button {
                icon.source: "qrc:/icons/icons/ui/screen-share.svg"
                text: qsTr("Share")
                visible: CallManager.screenShareReady

                onClicked: {
                    Settings.screenShareFrameRate = frameRateCombo.currentText;
                    Settings.screenSharePiP = pipCheckBox.checked;
                    Settings.screenShareRemoteVideo = remoteVideoCheckBox.checked;
                    Settings.screenShareHideCursor = hideCursorCheckBox.checked;
                    CallManager.sendInvite(room.roomId, Voip.SCREEN, windowCombo.currentIndex);
                    close();
                }
            }
            Button {
                text: qsTr("Preview")
                visible: CallManager.screenShareReady

                onClicked: {
                    CallManager.previewWindow(windowCombo.currentIndex);
                }
            }
            Button {
                text: qsTr("Cancel")

                onClicked: {
                    close();
                }
            }
        }
    }
}
