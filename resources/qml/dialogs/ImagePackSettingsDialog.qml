// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../components"
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import im.nheko 1.0

ApplicationWindow {
    id: win

    property Room room
    property ImagePackListModel packlist
    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 2.3)
    property SingleImagePackModel currentPack: packlist.packAt(currentPackIndex)
    property int currentPackIndex: 0
    readonly property int stickerDim: 128
    readonly property int stickerDimPad: 128 + Nheko.paddingSmall

    title: qsTr("Image pack settings")
    height: 600
    width: 800
    palette: Nheko.colors
    color: Nheko.colors.base
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

    Component {
        id: packEditor

        ImagePackEditorDialog {
        }

    }

    AdaptiveLayout {
        id: adaptiveView

        anchors.fill: parent
        singlePageMode: false
        pageIndex: 0

        AdaptiveLayoutElement {
            id: packlistC

            visible: Settings.groupView
            minimumWidth: 200
            collapsedWidth: 200
            preferredWidth: 300
            maximumWidth: 300

            ListView {
                model: packlist
                clip: true

                ScrollHelper {
                    flickable: parent
                    anchors.fill: parent
                    enabled: !Settings.mobileMode
                }

                footer: ColumnLayout {
                    Button {
                        palette: Nheko.colors
                        onClicked: {
                            var dialog = packEditor.createObject(timelineRoot, {
                                "imagePack": packlist.newPack(false)
                            });
                            dialog.show();
                            timelineRoot.destroyOnClose(dialog);
                        }
                        width: packlistC.width
                        visible: !packlist.containsAccountPack
                        text: qsTr("Create account pack")
                    }

                    Button {
                        palette: Nheko.colors
                        onClicked: {
                            var dialog = packEditor.createObject(timelineRoot, {
                                "imagePack": packlist.newPack(true)
                            });
                            dialog.show();
                            timelineRoot.destroyOnClose(dialog);
                        }
                        width: packlistC.width
                        visible: room.permissions.canChange(MtxEvent.ImagePackInRoom)
                        text: qsTr("New room pack")
                    }

                }

                delegate: AvatarListTile {
                    id: packItem

                    property color background: Nheko.colors.window
                    property color importantText: Nheko.colors.text
                    property color unimportantText: Nheko.colors.buttonText
                    property color bubbleBackground: Nheko.colors.highlight
                    property color bubbleText: Nheko.colors.highlightedText
                    required property string displayName
                    required property bool fromAccountData
                    required property bool fromCurrentRoom
                    required property bool fromSpace
                    required property string statekey

                    title: displayName
                    subtitle: {
                        if (fromAccountData)
                            return qsTr("Private pack");
                        else if (fromCurrentRoom)
                            return qsTr("Pack from this room");
                        else if (fromSpace)
                            return qsTr("Pack from parent community");
                        else
                            return qsTr("Globally enabled pack");
                    }
                    selectedIndex: currentPackIndex
                    roomid: statekey

                    TapHandler {
                        onSingleTapped: currentPackIndex = index
                    }

                }

            }

        }

        AdaptiveLayoutElement {
            id: packinfoC

            Rectangle {
                color: Nheko.colors.window

                ColumnLayout {
                    id: packinfo

                    property string packName: currentPack ? currentPack.packname : ""
                    property string attribution: currentPack ? currentPack.attribution : ""
                    property string avatarUrl: currentPack ? currentPack.avatarUrl : ""
                    property string statekey: currentPack ? currentPack.statekey : ""

                    anchors.fill: parent
                    anchors.margins: Nheko.paddingLarge
                    spacing: Nheko.paddingLarge

                    Avatar {
                        url: packinfo.avatarUrl.replace("mxc://", "image://MxcImage/")
                        displayName: packinfo.packName
                        roomid: packinfo.statekey
                        height: 100
                        width: 100
                        Layout.alignment: Qt.AlignHCenter
                        enabled: false
                    }

                    MatrixText {
                        text: packinfo.packName
                        font.pixelSize: Math.ceil(fontMetrics.pixelSize * 1.1)
                        horizontalAlignment: TextEdit.AlignHCenter
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: packinfoC.width - Nheko.paddingLarge * 2
                    }

                    MatrixText {
                        text: packinfo.attribution
                        wrapMode: TextEdit.Wrap
                        horizontalAlignment: TextEdit.AlignHCenter
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: packinfoC.width - Nheko.paddingLarge * 2
                    }

                    GridLayout {
                        Layout.alignment: Qt.AlignHCenter
                        visible: currentPack && currentPack.roomid != ""
                        columns: 2
                        rowSpacing: Nheko.paddingMedium

                        MatrixText {
                            text: qsTr("Enable globally")
                        }

                        ToggleButton {
                            ToolTip.text: qsTr("Enables this pack to be used in all rooms")
                            checked: currentPack ? currentPack.isGloballyEnabled : false
                            onCheckedChanged: currentPack.isGloballyEnabled = checked
                            Layout.alignment: Qt.AlignRight
                        }

                    }

                    Button {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Edit")
                        enabled: currentPack.canEdit
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
                        model: currentPack
                        cellWidth: stickerDimPad
                        cellHeight: stickerDimPad
                        boundsBehavior: Flickable.StopAtBounds
                        clip: true
                        currentIndex: -1 // prevent sorting from stealing focus
                        cacheBuffer: 500

                        ScrollHelper {
                            flickable: parent
                            anchors.fill: parent
                            enabled: !Settings.mobileMode
                        }

                        // Individual emoji
                        delegate: AbstractButton {
                            width: stickerDim
                            height: stickerDim
                            hoverEnabled: true
                            ToolTip.text: ":" + model.shortCode + ": - " + model.body
                            ToolTip.visible: hovered

                            contentItem: Image {
                                height: stickerDim
                                width: stickerDim
                                source: model.url.replace("mxc://", "image://MxcImage/") + "?scale"
                                fillMode: Image.PreserveAspectFit
                            }

                            background: Rectangle {
                                anchors.fill: parent
                                color: hovered ? Nheko.colors.highlight : 'transparent'
                                radius: 5
                            }

                        }

                    }

                }

            }

        }

    }

    footer: DialogButtonBox {
        id: buttons

        Button {
            text: qsTr("Close")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: win.close()
        }

    }

}
