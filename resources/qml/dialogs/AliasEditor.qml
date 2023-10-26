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

    property var roomSettings
    property var editingModel: Nheko.editAliases(roomSettings.roomId)

    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    minimumWidth: 300
    minimumHeight: 400
    height: 600
    width: 500

    title: qsTr("Aliases to %1").arg(roomSettings.roomName);

    //    Shortcut {
    //        sequence: StandardKey.Cancel
    //        onActivated: dbb.rejected()
    //    }

    ColumnLayout {
        anchors.margins: Nheko.paddingMedium
        anchors.fill: parent
        spacing: 0


        MatrixText {
            text: qsTr("List of aliases to this room. Usually you can only add aliases on your server. You can have one canonical alias and many alternate aliases.")
            font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.1)
            Layout.fillWidth: true
            Layout.fillHeight: false
            color: palette.text
            Layout.bottomMargin: Nheko.paddingMedium
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            id: view

            clip: true


            model: editingModel
            spacing: 4
            cacheBuffer: 50

            delegate: RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                Text {
                    Layout.fillWidth: true
                    text: model.name
                    color: model.isPublished ? palette.text : Nheko.theme.error
                    textFormat: Text.PlainText
                }

                ImageButton {
                    Layout.alignment: Qt.AlignRight
                    Layout.margins: 2
                    image: ":/icons/icons/ui/star.svg"
                    hoverEnabled: true
                    buttonTextColor: model.isCanonical ? palette.highlight : palette.text
                    highlightColor: editingModel.canAdvertize ? palette.highlight : buttonTextColor

                    ToolTip.visible: hovered
                    ToolTip.text: model.isCanonical ? qsTr("Primary alias") : qsTr("Make primary alias")

                    onClicked: editingModel.makeCanonical(model.index)
                }

                ImageButton {
                    Layout.alignment: Qt.AlignRight
                    Layout.margins: 2
                    image: ":/icons/icons/ui/building-shop.svg"
                    hoverEnabled: true
                    buttonTextColor: model.isAdvertized ? palette.highlight : palette.text
                    highlightColor: editingModel.canAdvertize ? palette.highlight : buttonTextColor

                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Advertise as an alias in this room")

                    onClicked: editingModel.toggleAdvertize(model.index)
                }

                ImageButton {
                    Layout.alignment: Qt.AlignRight
                    Layout.margins: 2
                    image: ":/icons/icons/ui/room-directory.svg"
                    hoverEnabled: true
                    buttonTextColor: model.isPublished ? palette.highlight : palette.text

                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Publish in room directory")

                    onClicked: editingModel.togglePublish(model.index)
                }

                ImageButton {
                    Layout.alignment: Qt.AlignRight
                    Layout.margins: 2
                    image: ":/icons/icons/ui/dismiss.svg"
                    hoverEnabled: true

                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Remove this alias")

                    onClicked: editingModel.deleteAlias(model.index)
                }
            }
        }

        RowLayout {
            spacing: Nheko.paddingMedium
            Layout.fillWidth: true

            MatrixTextField {
                id: newAliasVal

                focus: true
                Layout.fillWidth: true
                selectByMouse: true
                font.pixelSize: fontMetrics.font.pixelSize
                color: palette.text
                placeholderText: qsTr("#new-alias:server.tld")

                Component.onCompleted: forceActiveFocus()
                Keys.onPressed: {
                    if (event.matches(StandardKey.InsertParagraphSeparator)) {
                        editingModel.addAlias(newAliasVal.text);
                        newAliasVal.clear();
                    }
                }
            }

            Button {
                text: qsTr("Add")
                Layout.preferredWidth: 100
                onClicked: {
                    editingModel.addAlias(newAliasVal.text);
                    newAliasVal.clear();
                }
            }
        }
    }

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        onAccepted: {
            editingModel.commit();
            aliasEditorW.close();
        }
        onRejected: aliasEditorW.close();
    }

}
