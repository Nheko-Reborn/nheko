// SPDX-FileCopyrightText: 2021 Nheko Contributors
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

    property ImagePackListModel packlist
    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 2.3)
    property SingleImagePackModel currentPack: packlist.packAt(currentPackIndex)
    property int currentPackIndex: 0
    readonly property int stickerDim: 128
    readonly property int stickerDimPad: 128 + Nheko.paddingSmall

    title: qsTr("Image pack settings")
    height: 400
    width: 600
    palette: Nheko.colors
    color: Nheko.colors.base
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint
    Component.onCompleted: Nheko.reparent(win)

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

                delegate: Rectangle {
                    id: packItem

                    property color background: Nheko.colors.window
                    property color importantText: Nheko.colors.text
                    property color unimportantText: Nheko.colors.buttonText
                    property color bubbleBackground: Nheko.colors.highlight
                    property color bubbleText: Nheko.colors.highlightedText
                    required property string displayName
                    required property string avatarUrl
                    required property bool fromAccountData
                    required property bool fromCurrentRoom
                    required property int index

                    color: background
                    height: avatarSize + 2 * Nheko.paddingMedium
                    width: ListView.view.width
                    state: "normal"
                    states: [
                        State {
                            name: "highlight"
                            when: hovered.hovered && !(index == currentPackIndex)

                            PropertyChanges {
                                target: packItem
                                background: Nheko.colors.dark
                                importantText: Nheko.colors.brightText
                                unimportantText: Nheko.colors.brightText
                                bubbleBackground: Nheko.colors.highlight
                                bubbleText: Nheko.colors.highlightedText
                            }

                        },
                        State {
                            name: "selected"
                            when: index == currentPackIndex

                            PropertyChanges {
                                target: packItem
                                background: Nheko.colors.highlight
                                importantText: Nheko.colors.highlightedText
                                unimportantText: Nheko.colors.highlightedText
                                bubbleBackground: Nheko.colors.highlightedText
                                bubbleText: Nheko.colors.highlight
                            }

                        }
                    ]

                    TapHandler {
                        margin: -Nheko.paddingSmall
                        onSingleTapped: currentPackIndex = index
                    }

                    HoverHandler {
                        id: hovered
                    }

                    RowLayout {
                        spacing: Nheko.paddingMedium
                        anchors.fill: parent
                        anchors.margins: Nheko.paddingMedium

                        Avatar {
                            // In the future we could show an online indicator by setting the userid for the avatar
                            //userid: Nheko.currentUser.userid

                            id: avatar

                            enabled: false
                            Layout.alignment: Qt.AlignVCenter
                            height: avatarSize
                            width: avatarSize
                            url: avatarUrl.replace("mxc://", "image://MxcImage/")
                            displayName: packItem.displayName
                        }

                        ColumnLayout {
                            id: textContent

                            Layout.alignment: Qt.AlignLeft
                            Layout.fillWidth: true
                            Layout.minimumWidth: 100
                            width: parent.width - avatar.width
                            Layout.preferredWidth: parent.width - avatar.width
                            spacing: Nheko.paddingSmall

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                ElidedLabel {
                                    Layout.alignment: Qt.AlignBottom
                                    color: packItem.importantText
                                    elideWidth: textContent.width - Nheko.paddingMedium
                                    fullText: displayName
                                    textFormat: Text.PlainText
                                }

                                Item {
                                    Layout.fillWidth: true
                                }

                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                ElidedLabel {
                                    color: packItem.unimportantText
                                    font.pixelSize: fontMetrics.font.pixelSize * 0.9
                                    elideWidth: textContent.width - Nheko.paddingSmall
                                    fullText: {
                                        if (fromAccountData)
                                            return qsTr("Private pack");
                                        else if (fromCurrentRoom)
                                            return qsTr("Pack from this room");
                                        else
                                            return qsTr("Globally enabled pack");
                                    }
                                    textFormat: Text.PlainText
                                }

                                Item {
                                    Layout.fillWidth: true
                                }

                            }

                        }

                    }

                }

            }

        }

        AdaptiveLayoutElement {
            id: packinfoC

            Rectangle {
                color: Nheko.colors.window

                ColumnLayout {
                    //Button {
                    //    Layout.alignment: Qt.AlignHCenter
                    //    text: qsTr("Edit")
                    //    enabled: currentPack.canEdit
                    //}

                    id: packinfo

                    property string packName: currentPack ? currentPack.packname : ""
                    property string avatarUrl: currentPack ? currentPack.avatarUrl : ""

                    anchors.fill: parent
                    anchors.margins: Nheko.paddingLarge
                    spacing: Nheko.paddingLarge

                    Avatar {
                        url: packinfo.avatarUrl.replace("mxc://", "image://MxcImage/")
                        displayName: packinfo.packName
                        height: 100
                        width: 100
                        Layout.alignment: Qt.AlignHCenter
                        enabled: false
                    }

                    MatrixText {
                        text: packinfo.packName
                        font.pixelSize: 24
                        Layout.alignment: Qt.AlignHCenter
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
                            onClicked: currentPack.isGloballyEnabled = !currentPack.isGloballyEnabled
                            Layout.alignment: Qt.AlignRight
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
                            ToolTip.text: ":" + model.shortcode + ": - " + model.body
                            ToolTip.visible: hovered

                            contentItem: Image {
                                height: stickerDim
                                width: stickerDim
                                source: model.url.replace("mxc://", "image://MxcImage/")
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
