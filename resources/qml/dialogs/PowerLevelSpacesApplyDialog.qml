// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import im.nheko

ApplicationWindow {
    id: applyDialog

    property PowerlevelEditingModels editingModel
    property RoomSettings roomSettings

    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 680
    minimumHeight: 450
    minimumWidth: 340
    modality: Qt.NonModal
    title: qsTr("Apply permission changes")
    width: 450

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onAccepted: {
            editingModel.spaces.commit();
            applyDialog.close();
        }
        onRejected: applyDialog.close()
    }

    Shortcut {
        sequence: StandardKey.Cancel

        onActivated: roomSettingsDialog.close()
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: Nheko.paddingLarge

        MatrixText {
            Layout.bottomMargin: Nheko.paddingMedium
            Layout.fillHeight: false
            Layout.fillWidth: true
            color: palette.text
            font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.1)
            text: qsTr("Which of the subcommunities and rooms should these permissions be applied to?")
        }
        GridLayout {
            Layout.fillHeight: false
            Layout.fillWidth: true
            columns: 2

            Label {
                Layout.fillWidth: true
                color: palette.text
                text: qsTr("Apply permissions recursively")
            }
            ToggleButton {
                Layout.alignment: Qt.AlignRight
                checked: editingModel.spaces.applyToChildren

                onCheckedChanged: editingModel.spaces.applyToChildren = checked
            }
            Label {
                Layout.fillWidth: true
                color: palette.text
                text: qsTr("Overwrite exisiting modifications in rooms")
            }
            ToggleButton {
                Layout.alignment: Qt.AlignRight
                checked: editingModel.spaces.overwriteDiverged

                onCheckedChanged: editingModel.spaces.overwriteDiverged = checked
            }
        }
        ListView {
            id: view

            Layout.fillHeight: true
            Layout.fillWidth: true
            cacheBuffer: 50
            clip: true
            model: editingModel.spaces
            spacing: 4

            delegate: RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                ColumnLayout {
                    Layout.fillWidth: true

                    Text {
                        Layout.fillWidth: true
                        color: palette.text
                        elide: Text.ElideRight
                        text: model.displayName
                        textFormat: Text.PlainText
                    }
                    Text {
                        Layout.fillWidth: true
                        color: palette.buttonText
                        elide: Text.ElideRight
                        text: {
                            if (!model.isEditable)
                                return qsTr("No permissions to apply the new permissions here");
                            if (model.isAlreadyUpToDate)
                                return qsTr("No changes needed");
                            if (model.isDifferentFromBase)
                                return qsTr("Existing modifications to the permissions in this room will be overwritten");
                            return qsTr("Permissions synchronized with community");
                        }
                        textFormat: Text.PlainText
                    }
                }
                ToggleButton {
                    Layout.alignment: Qt.AlignRight
                    checked: model.applyPermissions
                    enabled: model.isEditable

                    onCheckedChanged: model.applyPermissions = checked
                }
            }
        }
    }
}
