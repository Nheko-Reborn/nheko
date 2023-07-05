// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Popup {
    modal: true

    anchors.centerIn: parent;

    Component.onCompleted: {
        frameRateCombo.currentIndex = frameRateCombo.find(Settings.screenShareFrameRate);
    }
    Component.onDestruction: {
        CallManager.closeScreenShare();
    }

    ColumnLayout {
        Label {
            Layout.topMargin: 16
            Layout.bottomMargin: 16
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Share desktop with %1?").arg(room.roomName)
            color: palette.windowText
        }

        RowLayout {
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.bottomMargin: 8

            Label {
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Method:")
            color: palette.windowText
            }

          ComboBox {
            id: screenshareType

            Layout.fillWidth: true
            model: CallManager.screenShareTypeList()
            onCurrentIndexChanged: CallManager.setScreenShareType(currentIndex);
          }
        }

        RowLayout {
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.bottomMargin: 8

            Label {
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Window:")
                color: palette.windowText
            }

            ComboBox {
                visible: CallManager.screenShareType == Voip.X11
                id: windowCombo

                Layout.fillWidth: true
                model: CallManager.windowList()
            }

            Button {
                visible: CallManager.screenShareType == Voip.XDP
                highlighted: !CallManager.screenShareReady
                text: qsTr("Request screencast")
                onClicked: {
                  Settings.screenShareHideCursor = hideCursorCheckBox.checked;
                  CallManager.setupScreenShareXDP();
                }
            }

        }

        RowLayout {
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.bottomMargin: 8

            Label {
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Frame rate:")
                color: palette.windowText
            }

            ComboBox {
                id: frameRateCombo

                Layout.fillWidth: true
                model: ["25", "20", "15", "10", "5", "2", "1"]
            }

        }

        GridLayout {
            columns: 2
            rowSpacing: 10
            Layout.margins: 8

            MatrixText {
                text: qsTr("Include your camera picture-in-picture")
            }

            ToggleButton {
                id: pipCheckBox

                enabled: CallManager.cameras.length > 0
                checked: CallManager.cameras.length > 0 && Settings.screenSharePiP
                Layout.alignment: Qt.AlignRight
            }

            MatrixText {
                text: qsTr("Request remote camera")
                ToolTip.text: qsTr("View your callee's camera like a regular video call")
                ToolTip.visible: hovered
            }

            ToggleButton {
                id: remoteVideoCheckBox

                Layout.alignment: Qt.AlignRight
                checked: Settings.screenShareRemoteVideo
                ToolTip.text: qsTr("View your callee's camera like a regular video call")
                ToolTip.visible: hovered
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
                visible: CallManager.screenShareReady
                text: qsTr("Share")
                icon.source: "qrc:/icons/icons/ui/screen-share.svg"

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
                visible: CallManager.screenShareReady
                text: qsTr("Preview")
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

    background: Rectangle {
        color: palette.window
        border.color: palette.windowText
    }

}
