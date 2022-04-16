// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import "../components"
import Qt.labs.platform 1.1
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import im.nheko

ApplicationWindow {
    id: win

    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 2.3)
    property int currentImageIndex: -1
    property SingleImagePackModel imagePack
    readonly property int stickerDim: 128
    readonly property int stickerDimPad: 128 + Nheko.paddingSmall

    color: timelineRoot.palette.base
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 600
    modality: Qt.WindowModal
    palette: timelineRoot.palette
    title: qsTr("Editing image pack")
    width: 600

    footer: DialogButtonBox {
        id: buttons
        standardButtons: DialogButtonBox.Save | DialogButtonBox.Cancel

        onAccepted: {
            imagePack.save();
            win.close();
        }
        onRejected: win.close()
    }

    AdaptiveLayout {
        id: adaptiveView
        anchors.fill: parent
        pageIndex: 0
        singlePageMode: false

        AdaptiveLayoutElement {
            id: packlistC
            clip: true
            collapsedWidth: 200
            maximumWidth: 300
            minimumWidth: 200
            preferredWidth: 300
            visible: Settings.groupView

            ListView {
                //required property bool isEmote
                //required property bool isSticker
                model: imagePack

                delegate: AvatarListTile {
                    id: packItem

                    property color background: timelineRoot.palette.window
                    required property string body
                    property color bubbleBackground: timelineRoot.palette.highlight
                    property color bubbleText: timelineRoot.palette.highlightedText
                    property color importantText: timelineRoot.palette.text
                    required property string shortCode
                    property color unimportantText: timelineRoot.palette.placeholderText
                    required property string url

                    avatarUrl: url
                    crop: false
                    selectedIndex: currentImageIndex
                    subtitle: body
                    title: shortCode

                    TapHandler {
                        onSingleTapped: currentImageIndex = index
                    }
                }
                footer: Button {
                    palette: timelineRoot.palette
                    text: qsTr("Add images")
                    width: ListView.view.width

                    onClicked: addFilesDialog.open()

                    FileDialog {
                        id: addFilesDialog
                        acceptLabel: qsTr("Add to pack")
                        fileMode: FileDialog.OpenFiles
                        folder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
                        nameFilters: [qsTr("Images (*.png *.webp *.gif *.jpg *.jpeg)")]
                        title: qsTr("Select images for pack")

                        onAccepted: imagePack.addStickers(files)
                    }
                }
                header: AvatarListTile {
                    avatarUrl: imagePack.avatarUrl
                    index: -1
                    roomid: imagePack.statekey
                    selectedIndex: currentImageIndex
                    subtitle: imagePack.statekey
                    title: imagePack.packname

                    TapHandler {
                        onSingleTapped: currentImageIndex = -1
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        color: timelineRoot.palette.highlight
                        height: parent.height - Nheko.paddingSmall * 2
                        width: 3
                    }
                }
            }
        }
        AdaptiveLayoutElement {
            id: packinfoC
            Rectangle {
                color: timelineRoot.palette.window

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: Nheko.paddingMedium
                    columns: 2
                    enabled: visible
                    rowSpacing: Nheko.paddingLarge
                    visible: currentImageIndex == -1

                    Avatar {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.columnSpan: 2
                        crop: false
                        displayName: imagePack.packname
                        height: 130
                        roomid: imagePack.statekey
                        url: imagePack.avatarUrl.replace("mxc://", "image://MxcImage/")
                        width: 130

                        ImageButton {
                            ToolTip.text: qsTr("Change the overview image for this pack")
                            ToolTip.visible: hovered
                            anchors.left: parent.left
                            anchors.leftMargin: Nheko.paddingMedium
                            anchors.top: parent.top
                            anchors.topMargin: Nheko.paddingMedium
                            hoverEnabled: true
                            image: ":/icons/icons/ui/edit.svg"

                            onClicked: addAvatarDialog.open()

                            FileDialog {
                                id: addAvatarDialog
                                fileMode: FileDialog.OpenFile
                                folder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
                                nameFilters: [qsTr("Overview Image (*.png *.webp *.jpg *.jpeg)")]
                                title: qsTr("Select overview image for pack")

                                onAccepted: imagePack.setAvatar(file)
                            }
                        }
                    }
                    MatrixTextField {
                        id: statekeyField
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        label: qsTr("State key")
                        text: imagePack.statekey
                        visible: imagePack.roomid

                        onTextEdited: imagePack.statekey = text
                    }
                    MatrixTextField {
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        label: qsTr("Packname")
                        text: imagePack.packname

                        onTextEdited: imagePack.packname = text
                    }
                    MatrixTextField {
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        label: qsTr("Attribution")
                        text: imagePack.attribution

                        onTextEdited: imagePack.attribution = text
                    }
                    MatrixText {
                        Layout.margins: statekeyField.textPadding
                        font.letterSpacing: font.pixelSize * 0.02
                        font.weight: Font.DemiBold
                        text: qsTr("Use as Emoji")
                    }
                    ToggleButton {
                        Layout.alignment: Qt.AlignRight
                        checked: imagePack.isEmotePack

                        onCheckedChanged: imagePack.isEmotePack = checked
                    }
                    MatrixText {
                        Layout.margins: statekeyField.textPadding
                        font.letterSpacing: font.pixelSize * 0.02
                        font.weight: Font.DemiBold
                        text: qsTr("Use as Sticker")
                    }
                    ToggleButton {
                        Layout.alignment: Qt.AlignRight
                        checked: imagePack.isStickerPack

                        onCheckedChanged: imagePack.isStickerPack = checked
                    }
                    Item {
                        Layout.columnSpan: 2
                        Layout.fillHeight: true
                    }
                }
                GridLayout {
                    anchors.fill: parent
                    anchors.margins: Nheko.paddingMedium
                    columns: 2
                    enabled: visible
                    rowSpacing: Nheko.paddingLarge
                    visible: currentImageIndex >= 0

                    Avatar {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.columnSpan: 2
                        crop: false
                        displayName: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.ShortCode)
                        height: 130
                        roomid: displayName
                        url: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.Url).replace("mxc://", "image://MxcImage/") + "?scale"
                        width: 130
                    }
                    MatrixTextField {
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        label: qsTr("Shortcode")
                        text: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.ShortCode)

                        onTextEdited: imagePack.setData(imagePack.index(currentImageIndex, 0), text, SingleImagePackModel.ShortCode)
                    }
                    MatrixTextField {
                        id: bodyField
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        label: qsTr("Body")
                        text: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.Body)

                        onTextEdited: imagePack.setData(imagePack.index(currentImageIndex, 0), text, SingleImagePackModel.Body)
                    }
                    MatrixText {
                        Layout.margins: bodyField.textPadding
                        font.letterSpacing: font.pixelSize * 0.02
                        font.weight: Font.DemiBold
                        text: qsTr("Use as Emoji")
                    }
                    ToggleButton {
                        Layout.alignment: Qt.AlignRight
                        checked: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.IsEmote)

                        onCheckedChanged: imagePack.setData(imagePack.index(currentImageIndex, 0), checked, SingleImagePackModel.IsEmote)
                    }
                    MatrixText {
                        Layout.margins: bodyField.textPadding
                        font.letterSpacing: font.pixelSize * 0.02
                        font.weight: Font.DemiBold
                        text: qsTr("Use as Sticker")
                    }
                    ToggleButton {
                        Layout.alignment: Qt.AlignRight
                        checked: imagePack.data(imagePack.index(currentImageIndex, 0), SingleImagePackModel.IsSticker)

                        onCheckedChanged: imagePack.setData(imagePack.index(currentImageIndex, 0), checked, SingleImagePackModel.IsSticker)
                    }
                    MatrixText {
                        Layout.margins: bodyField.textPadding
                        font.letterSpacing: font.pixelSize * 0.02
                        font.weight: Font.DemiBold
                        text: qsTr("Remove from pack")
                    }
                    Button {
                        Layout.alignment: Qt.AlignRight
                        text: qsTr("Remove")

                        onClicked: {
                            let temp = currentImageIndex;
                            currentImageIndex = -1;
                            imagePack.remove(temp);
                        }
                    }
                    Item {
                        Layout.columnSpan: 2
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }
}
