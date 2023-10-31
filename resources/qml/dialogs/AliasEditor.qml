// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

ApplicationWindow {
    id: aliasEditorW

    property var editingModel: Nheko.editAliases(roomSettings.roomId)
    property var roomSettings

    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 600
    minimumHeight: 400
    minimumWidth: 300
    modality: Qt.NonModal
    title: qsTr("Aliases to %1").arg(roomSettings.roomName)
    width: 500

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onAccepted: {
            editingModel.commit();
            aliasEditorW.close();
        }
        onRejected: aliasEditorW.close()
    }

    //    Shortcut {
    //        sequence: StandardKey.Cancel
    //        onActivated: dbb.rejected()
    //    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        spacing: 0

        MatrixText {
            Layout.bottomMargin: Nheko.paddingMedium
            Layout.fillHeight: false
            Layout.fillWidth: true
            color: palette.text
            font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.1)
            text: qsTr("List of aliases to this room. Usually you can only add aliases on your server. You can have one canonical alias and many alternate aliases.")
        }
        ListView {
            id: view

            Layout.fillHeight: true
            Layout.fillWidth: true
            cacheBuffer: 50
            clip: true
            model: editingModel
            spacing: 4

            delegate: RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                Text {
                    Layout.fillWidth: true
                    color: model.isPublished ? palette.text : Nheko.theme.error
                    text: model.name
                    textFormat: Text.PlainText
                }
                ImageButton {
                    Layout.alignment: Qt.AlignRight
                    Layout.margins: 2
                    ToolTip.text: model.isCanonical ? qsTr("Primary alias") : qsTr("Make primary alias")
                    ToolTip.visible: hovered
                    buttonTextColor: model.isCanonical ? palette.highlight : palette.text
                    highlightColor: editingModel.canAdvertize ? palette.highlight : buttonTextColor
                    hoverEnabled: true
                    image: ":/icons/icons/ui/star.svg"

                    onClicked: editingModel.makeCanonical(model.index)
                }
                ImageButton {
                    Layout.alignment: Qt.AlignRight
                    Layout.margins: 2
                    ToolTip.text: qsTr("Advertise as an alias in this room")
                    ToolTip.visible: hovered
                    buttonTextColor: model.isAdvertized ? palette.highlight : palette.text
                    highlightColor: editingModel.canAdvertize ? palette.highlight : buttonTextColor
                    hoverEnabled: true
                    image: ":/icons/icons/ui/building-shop.svg"

                    onClicked: editingModel.toggleAdvertize(model.index)
                }
                ImageButton {
                    Layout.alignment: Qt.AlignRight
                    Layout.margins: 2
                    ToolTip.text: qsTr("Publish in room directory")
                    ToolTip.visible: hovered
                    buttonTextColor: model.isPublished ? palette.highlight : palette.text
                    hoverEnabled: true
                    image: ":/icons/icons/ui/room-directory.svg"

                    onClicked: editingModel.togglePublish(model.index)
                }
                ImageButton {
                    Layout.alignment: Qt.AlignRight
                    Layout.margins: 2
                    ToolTip.text: qsTr("Remove this alias")
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/dismiss.svg"

                    onClicked: editingModel.deleteAlias(model.index)
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Nheko.paddingMedium

            MatrixTextField {
                id: newAliasVal

                Layout.fillWidth: true
                color: palette.text
                focus: true
                font.pixelSize: fontMetrics.font.pixelSize
                placeholderText: qsTr("#new-alias:server.tld")
                selectByMouse: true

                Component.onCompleted: forceActiveFocus()
                Keys.onPressed: {
                    if (event.matches(StandardKey.InsertParagraphSeparator)) {
                        editingModel.addAlias(newAliasVal.text);
                        newAliasVal.clear();
                    }
                }
            }
            Button {
                Layout.preferredWidth: 100
                text: qsTr("Add")

                onClicked: {
                    editingModel.addAlias(newAliasVal.text);
                    newAliasVal.clear();
                }
            }
        }
    }
}
