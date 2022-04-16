// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import "../components"
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import im.nheko

ApplicationWindow {
    id: win

    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 2.3)
    property SingleImagePackModel currentPack: packlist.packAt(currentPackIndex)
    property int currentPackIndex: 0
    property ImagePackListModel packlist
    property Room room
    readonly property int stickerDim: 128
    readonly property int stickerDimPad: 128 + Nheko.paddingSmall

    color: timelineRoot.palette.base
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 600
    modality: Qt.NonModal
    palette: timelineRoot.palette
    title: qsTr("Image pack settings")
    width: 800

    footer: DialogButtonBox {
        id: buttons
        Button {
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            text: qsTr("Close")

            onClicked: win.close()
        }
    }

    Component {
        id: packEditor
        ImagePackEditorDialog {
        }
    }
    AdaptiveLayout {
        id: adaptiveView
        anchors.fill: parent
        pageIndex: 0
        singlePageMode: false

        AdaptiveLayoutElement {
            id: packlistC
            collapsedWidth: 200
            maximumWidth: 300
            minimumWidth: 200
            preferredWidth: 300
            visible: Settings.groupView

            ListView {
                clip: true
                model: packlist

                delegate: AvatarListTile {
                    id: packItem

                    property color background: timelineRoot.palette.window
                    property color bubbleBackground: timelineRoot.palette.highlight
                    property color bubbleText: timelineRoot.palette.highlightedText
                    required property string displayName
                    required property bool fromAccountData
                    required property bool fromCurrentRoom
                    property color importantText: timelineRoot.palette.text
                    required property string statekey
                    property color unimportantText: timelineRoot.palette.placeholderText

                    roomid: statekey
                    selectedIndex: currentPackIndex
                    subtitle: {
                        if (fromAccountData)
                            return qsTr("Private pack");
                        else if (fromCurrentRoom)
                            return qsTr("Pack from this room");
                        else
                            return qsTr("Globally enabled pack");
                    }
                    title: displayName

                    TapHandler {
                        onSingleTapped: currentPackIndex = index
                    }
                }
                footer: ColumnLayout {
                    Button {
                        palette: timelineRoot.palette
                        text: qsTr("Create account pack")
                        visible: !packlist.containsAccountPack
                        width: packlistC.width

                        onClicked: {
                            var dialog = packEditor.createObject(timelineRoot, {
                                    "imagePack": packlist.newPack(false)
                                });
                            dialog.show();
                            timelineRoot.destroyOnClose(dialog);
                        }
                    }
                    Button {
                        palette: timelineRoot.palette
                        text: qsTr("New room pack")
                        visible: room.permissions.canChange(MtxEvent.ImagePackInRoom)
                        width: packlistC.width

                        onClicked: {
                            var dialog = packEditor.createObject(timelineRoot, {
                                    "imagePack": packlist.newPack(true)
                                });
                            dialog.show();
                            timelineRoot.destroyOnClose(dialog);
                        }
                    }
                }
            }
        }
        AdaptiveLayoutElement {
            id: packinfoC
            Rectangle {
                color: timelineRoot.palette.window

                ColumnLayout {
                    id: packinfo

                    property string attribution: currentPack ? currentPack.attribution : ""
                    property string avatarUrl: currentPack ? currentPack.avatarUrl : ""
                    property string packName: currentPack ? currentPack.packname : ""
                    property string statekey: currentPack ? currentPack.statekey : ""

                    anchors.fill: parent
                    anchors.margins: Nheko.paddingLarge
                    spacing: Nheko.paddingLarge

                    Avatar {
                        Layout.alignment: Qt.AlignHCenter
                        displayName: packinfo.packName
                        enabled: false
                        height: 100
                        roomid: packinfo.statekey
                        url: packinfo.avatarUrl.replace("mxc://", "image://MxcImage/")
                        width: 100
                    }
                    MatrixText {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: packinfoC.width - Nheko.paddingLarge * 2
                        font.pixelSize: Math.ceil(fontMetrics.pixelSize * 1.1)
                        horizontalAlignment: TextEdit.AlignHCenter
                        text: packinfo.packName
                    }
                    MatrixText {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: packinfoC.width - Nheko.paddingLarge * 2
                        horizontalAlignment: TextEdit.AlignHCenter
                        text: packinfo.attribution
                        wrapMode: TextEdit.Wrap
                    }
                    GridLayout {
                        Layout.alignment: Qt.AlignHCenter
                        columns: 2
                        rowSpacing: Nheko.paddingMedium
                        visible: currentPack && currentPack.roomid != ""

                        MatrixText {
                            text: qsTr("Enable globally")
                        }
                        ToggleButton {
                            Layout.alignment: Qt.AlignRight
                            ToolTip.text: qsTr("Enables this pack to be used in all rooms")
                            checked: currentPack ? currentPack.isGloballyEnabled : false

                            onCheckedChanged: currentPack.isGloballyEnabled = checked
                        }
                    }
                    Button {
                        Layout.alignment: Qt.AlignHCenter
                        enabled: currentPack.canEdit
                        text: qsTr("Edit")

                        onClicked: {
                            var dialog = packEditor.createObject(timelineRoot, {
                                    "imagePack": currentPack
                                });
                            dialog.show();
                            timelineRoot.destroyOnClose(dialog);
                        }
                    }
                    GridView {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        boundsBehavior: Flickable.StopAtBounds
                        cacheBuffer: 500
                        cellHeight: stickerDimPad
                        cellWidth: stickerDimPad
                        clip: true
                        currentIndex: -1 // prevent sorting from stealing focus
                        model: currentPack

                        // Individual emoji
                        delegate: AbstractButton {
                            ToolTip.text: ":" + model.shortCode + ": - " + model.body
                            ToolTip.visible: hovered
                            height: stickerDim
                            hoverEnabled: true
                            width: stickerDim

                            background: Rectangle {
                                anchors.fill: parent
                                color: hovered ? timelineRoot.palette.highlight : 'transparent'
                                radius: 5
                            }
                            contentItem: Image {
                                fillMode: Image.PreserveAspectFit
                                height: stickerDim
                                source: model.url.replace("mxc://", "image://MxcImage/") + "?scale"
                                width: stickerDim
                            }
                        }
                    }
                }
            }
        }
    }
}
