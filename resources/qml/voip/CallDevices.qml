// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Popup {
    modal: true
    palette: Nheko.colors
    // only set the anchors on Qt 5.12 or higher
    // see https://doc.qt.io/qt-5/qml-qtquick-controls2-popup.html#anchors.centerIn-prop
    Component.onCompleted: {
        if (anchors)
            anchors.centerIn = parent;

    }

    ColumnLayout {
        spacing: 16

        ColumnLayout {
            spacing: 8
            Layout.topMargin: 8
            Layout.leftMargin: 8
            Layout.rightMargin: 8

            RowLayout {
                Image {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    source: "image://colorimage/:/icons/icons/ui/microphone-unmute.svg?" + Nheko.colors.windowText
                }

                ComboBox {
                    id: micCombo

                    Layout.fillWidth: true
                    model: CallManager.mics
                }

            }

            RowLayout {
                visible: CallManager.callType == CallType.VIDEO && CallManager.cameras.length > 0

                Image {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    source: "image://colorimage/:/icons/icons/ui/video-call.svg?" + Nheko.colors.windowText
                }

                ComboBox {
                    id: cameraCombo

                    Layout.fillWidth: true
                    model: CallManager.cameras
                }

            }

        }

        DialogButtonBox {
            Layout.leftMargin: 128
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
            onAccepted: {
                Settings.microphone = micCombo.currentText;
                if (cameraCombo.visible)
                    Settings.camera = cameraCombo.currentText;

                close();
            }
            onRejected: {
                close();
            }
        }

    }

    background: Rectangle {
        color: Nheko.colors.window
        border.color: Nheko.colors.windowText
    }

}
