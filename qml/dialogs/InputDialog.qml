// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko

ApplicationWindow {
    id: inputDialog

    property alias echoMode: statusInput.echoMode
    property var onAccepted: undefined
    property alias prompt: promptLabel.text

    function forceActiveFocus() {
        statusInput.forceActiveFocus();
    }

    flags: Qt.Dialog
    height: fontMetrics.lineSpacing * 7
    modality: Qt.NonModal
    width: 350

    footer: DialogButtonBox {
        id: dbb
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

    Shortcut {
        sequence: StandardKey.Cancel

        onActivated: dbb.rejected()
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingMedium

        Label {
            id: promptLabel
            color: timelineRoot.palette.text
        }
        MatrixTextField {
            id: statusInput
            Layout.fillWidth: true
            focus: true

            onAccepted: dbb.accepted()
        }
    }
}
