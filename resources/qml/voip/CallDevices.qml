// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Popup {
    modal: true

    background: Rectangle {
        border.color: palette.windowText
        color: palette.window
    }

    // only set the anchors on Qt 5.12 or higher
    // see https://doc.qt.io/qt-5/qml-qtquick-controls2-popup.html#anchors.centerIn-prop
    Component.onCompleted: {
        if (anchors)
            anchors.centerIn = parent;
    }

    ColumnLayout {
        spacing: 16

        ColumnLayout {
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.topMargin: 8
            spacing: 8

            RowLayout {
                Image {
                    Layout.preferredHeight: 22
                    Layout.preferredWidth: 22
                    source: "image://colorimage/:/icons/icons/ui/microphone-unmute.svg?" + palette.windowText
                }
                ComboBox {
                    id: micCombo

                    Layout.fillWidth: true
                    model: CallManager.mics
                }
            }
            RowLayout {
                visible: CallManager.callType == Voip.VIDEO && CallManager.cameras.length > 0

                Image {
                    Layout.preferredHeight: 22
                    Layout.preferredWidth: 22
                    source: "image://colorimage/:/icons/icons/ui/video-call.svg?" + palette.windowText
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
}
