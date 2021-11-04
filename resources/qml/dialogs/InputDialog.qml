// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko 1.0

ApplicationWindow {
    id: inputDialog

    property alias prompt: promptLabel.text
    property alias echoMode: statusInput.echoMode
    property var onAccepted: undefined

    modality: Qt.NonModal
    flags: Qt.Dialog
    Component.onCompleted: Nheko.reparent(inputDialog)
    width: 350
    height: fontMetrics.lineSpacing * 7

    ColumnLayout {
        spacing: Nheko.paddingMedium
        anchors.margins: Nheko.paddingMedium
        anchors.fill: parent

        Label {
            id: promptLabel

            color: Nheko.colors.text
        }

        MatrixTextField {
            id: statusInput

            Layout.fillWidth: true
        }

    }

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        onAccepted: {
            if (inputDialog.onAccepted)
                inputDialog.onAccepted(statusInput.text);

            inputDialog.close();
        }
        onRejected: {
            inputDialog.close();
        }
    }

}
