// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../components"
import Qt.labs.platform 1.1
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import im.nheko 1.0

ApplicationWindow {
    id: win

    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 2.3)
    property SingleImagePackModel imagePack
    property int currentImageIndex: -1
    readonly property int stickerDim: 128
    readonly property int stickerDimPad: 128 + Nheko.paddingSmall

    title: qsTr("Editing image pack")
    height: 600
    width: 600
    palette: Nheko.colors
    color: Nheko.colors.base
    modality: Qt.WindowModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

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
            clip: true

            ListView {
                //required property bool isEmote
                //required property bool isSticker

                model: imagePack

                ScrollHelper {
                    flickable: parent
                    anchors.fill: parent
                    enabled: !Settings.mobileMode
                }

                header: AvatarListTile {
                    title: imagePack.packname
                    avatarUrl: imagePack.avatarUrl
                    roomid: imagePack.statekey
                    subtitle: imagePack.statekey
                    index: -1
                    selectedIndex: currentImageIndex

                    TapHandler {
                        onSingleTapped: currentImageIndex = -1
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        height: parent.height - Nheko.paddingSmall * 2
                        width: 3
                        color: Nheko.colors.highlight
                    }

                }

                footer: Button {
                    palette: Nheko.colors
                    onClicked: addFilesDialog.open()
                    width: ListView.view.width
                    text: qsTr("Add images")

                    FileDialog {
                        id: addFilesDialog

                        folder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
                        fileMode: FileDialog.OpenFiles
                        nameFilters: [qsTr("Images (*.png *.webp *.gif *.jpg *.jpeg)")]
                        title: qsTr("Select images for pack")
                        acceptLabel: qsTr("Add to pack")
                        onAccepted: imagePack.addStickers(files)
                    }

                }

                delegate: AvatarListTile {
                    id: packItem

                    property color background: Nheko.colors.window
                    property color importantText: Nheko.colors.text
                    property color unimportantText: Nheko.colors.buttonText
                    property color bubbleBackground: Nheko.colors.highlight
                    property color bubbleText: Nheko.colors.highlightedText
                    required property string shortCode
                    required property string url
                    required property string body

                    title: shortCode
                    subtitle: body
                    avatarUrl: url
                    selectedIndex: currentImageIndex
                    crop: false

                    TapHandler {
                        onSingleTapped: currentImageIndex = index
                    }

                }

            }

        }

        AdaptiveLayoutElement {
            id: packinfoC

            Rectangle {
                color: Nheko.colors.window

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: Nheko.paddingMedium
                    visible: currentImageIndex == -1
                    enabled: visible
                    columns: 2
                    rowSpacing: Nheko.paddingLarge

                    Avatar {
                        Layout.columnSpan: 2
                        url: imagePack.avatarUrl.replace("mxc://", "image://MxcImage/")
                        displayName: imagePack.packname
                        roomid: imagePack.statekey
                        height: 130
                        width: 130
                        crop: false
                        Layout.alignment: Qt.AlignHCenter

                        ImageButton {
                            hoverEnabled: true
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Change the overview image for this pack")
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.leftMargin: Nheko.paddingMedium
                            anchors.topMargin: Nheko.paddingMedium
                            image: ":/icons/icons/ui/edit.svg"
                            onClicked: addAvatarDialog.open()

                            FileDialog {
                                id: addAvatarDialog

                                folder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
                                fileMode: FileDialog.OpenFile
                                nameFilters: [qsTr("Overview Image (*.png *.webp *.jpg *.jpeg)")]
                                title: qsTr("Select overview image for pack")
                                onAccepted: imagePack.setAvatar(file)
                            }
                        }
                    }

                    MatrixTextField {
                        id: statekeyField

                        visible: imagePack.roomid
                        Layout.fillWidth: true
                        Layout.columnSpan: 2
                        label: qsTr("State key")
                        text: imagePack.statekey
                        onTextEdited: imagePack.statekey = text
                    }

                    MatrixTextField {
                        Layout.fillWidth: true
                        Layout.columnSpan: 2
                        label: qsTr("Packname")
                        text: imagePack.packname
                        onTextEdited: imagePack.packname = text
                    }

                    MatrixTextField {
                        Layout.fillWidth: true
                        Layout.columnSpan: 2
                        label: qsTr("Attribution")
                        text: imagePack.attribution
                        onTextEdited: imagePack.attribution = text
                    }

                    MatrixText {
                        Layout.margins: statekeyField.textPadding
                        font.weight: Font.DemiBold
                        font.letterSpacing: font.pixelSize * 0.02
                        text: qsTr("Use as Emoji")
                    }

                    ToggleButton {
                        checked: imagePack.isEmotePack
                        onCheckedChanged: imagePack.isEmotePack = checked
                        Layout.alignment: Qt.AlignRight
                    }

                    MatrixText {
                        Layout.margins: statekeyField.textPadding
                        font.weight: Font.DemiBold
                        font.letterSpacing: font.pixelSize * 0.02
                        text: qsTr("Use as Sticker")
                    }

                    ToggleButton {
                        checked: imagePack.isStickerPack
                        onCheckedChanged: imagePack.isStickerPack = checked
                        Layout.alignment: Qt.AlignRight
                    }

                    Item {
                        Layout.columnSpan: 2
                        Layout.fillHeight: true
                    }

                }

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: Nheko.paddingMedium
                    visible: currentImageIndex >= 0
                    enabled: visible
                    columns: 2
                    rowSpacing: Nheko.paddingLarge

                    Avatar {
                        Layout.columnSpan: 2
                        url: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.Url).replace("mxc://", "image://MxcImage/") + "?scale"
                        displayName: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.ShortCode)
                        roomid: displayName
                        height: 130
                        width: 130
                        crop: false
                        Layout.alignment: Qt.AlignHCenter
                    }

                    MatrixTextField {
                        Layout.fillWidth: true
                        Layout.columnSpan: 2
                        label: qsTr("Shortcode")
                        text: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.ShortCode)
                        onTextEdited: imagePack.setData(imagePack.index(currentImageIndex, 0), text, SingleImagePackModel.ShortCode)
                    }

                    MatrixTextField {
                        id: bodyField

                        Layout.fillWidth: true
                        Layout.columnSpan: 2
                        label: qsTr("Body")
                        text: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.Body)
                        onTextEdited: imagePack.setData(imagePack.index(currentImageIndex, 0), text, SingleImagePackModel.Body)
                    }

                    MatrixText {
                        Layout.margins: bodyField.textPadding
                        font.weight: Font.DemiBold
                        font.letterSpacing: font.pixelSize * 0.02
                        text: qsTr("Use as Emoji")
                    }

                    ToggleButton {
                        checked: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.IsEmote)
                        onCheckedChanged: imagePack.setData(imagePack.index(currentImageIndex, 0), checked, SingleImagePackModel.IsEmote)
                        Layout.alignment: Qt.AlignRight
                    }

                    MatrixText {
                        Layout.margins: bodyField.textPadding
                        font.weight: Font.DemiBold
                        font.letterSpacing: font.pixelSize * 0.02
                        text: qsTr("Use as Sticker")
                    }

                    ToggleButton {
                        checked: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.IsSticker)
                        onCheckedChanged: imagePack.setData(imagePack.index(currentImageIndex, 0), checked, SingleImagePackModel.IsSticker)
                        Layout.alignment: Qt.AlignRight
                    }

                    MatrixText {
                        Layout.margins: bodyField.textPadding
                        font.weight: Font.DemiBold
                        font.letterSpacing: font.pixelSize * 0.02
                        text: qsTr("Remove from pack")
                    }

                    Button {
                        text: qsTr("Remove")
                        onClicked: {
                            let temp = currentImageIndex;
                            currentImageIndex = -1;
                            imagePack.remove(temp);
                        }
                        Layout.alignment: Qt.AlignRight
                    }

                    Item {
                        Layout.columnSpan: 2
                        Layout.fillHeight: true
                    }

                }

            }

        }

    }

    footer: DialogButtonBox {
        id: buttons

        standardButtons: DialogButtonBox.Save | DialogButtonBox.Cancel
        onAccepted: {
            imagePack.save();
            win.close();
        }
        onRejected: win.close()
    }

}
