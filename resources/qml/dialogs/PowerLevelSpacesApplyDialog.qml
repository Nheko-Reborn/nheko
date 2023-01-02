// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../ui"
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0

ApplicationWindow {
    id: applyDialog

    property RoomSettings roomSettings
    property PowerlevelEditingModels editingModel

    minimumWidth: 340
    minimumHeight: 450
    width: 450
    height: 680
    palette: Nheko.colors
    color: Nheko.colors.window
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    title: qsTr("Apply permission changes")

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: roomSettingsDialog.close()
    }

    ColumnLayout {
        anchors.margins: Nheko.paddingMedium
        anchors.fill: parent
        spacing: Nheko.paddingLarge


        MatrixText {
            text: qsTr("Which of the subcommunities and rooms should these permissions be applied to?")
            font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.1)
            Layout.fillWidth: true
            Layout.fillHeight: false
            color: Nheko.colors.text
            Layout.bottomMargin: Nheko.paddingMedium
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: false
            columns: 2

                Label {
                    text: qsTr("Apply permissions recursively")
                    Layout.fillWidth: true
                    color: Nheko.colors.text
                }

                ToggleButton {
                    checked: editingModel.spaces.applyToChildren
                    Layout.alignment: Qt.AlignRight
                    onCheckedChanged: editingModel.spaces.applyToChildren = checked
                }

                Label {
                    text: qsTr("Overwrite exisiting modifications in rooms")
                    Layout.fillWidth: true
                    color: Nheko.colors.text
                }

                ToggleButton {
                    checked: editingModel.spaces.overwriteDiverged
                    Layout.alignment: Qt.AlignRight
                    onCheckedChanged: editingModel.spaces.overwriteDiverged = checked
                }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            id: view

            clip: true

            ScrollHelper {
                flickable: parent
                anchors.fill: parent
            }

            model: editingModel.spaces
            spacing: 4
            cacheBuffer: 50

            delegate: RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                ColumnLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: model.displayName
                        color: Nheko.colors.text
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: {
                            if (!model.isEditable) return qsTr("No permissions to apply the new permissions here");
                            if (model.isAlreadyUpToDate) return qsTr("No changes needed");
                            if (model.isDifferentFromBase) return qsTr("Existing modifications to the permissions in this room will be overwritten");
                            return qsTr("Permissions synchronized with community")
                        }
                        elide: Text.ElideRight
                        color: Nheko.colors.buttonText
                        textFormat: Text.PlainText
                    }
                }

                ToggleButton {
                    checked: model.applyPermissions
                    Layout.alignment: Qt.AlignRight
                    onCheckedChanged: model.applyPermissions = checked
                    enabled: model.isEditable
                }
            }
        }


    }

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        onAccepted: {
            editingModel.spaces.commit();
            applyDialog.close();
        }
        onRejected: applyDialog.close()
    }

}
