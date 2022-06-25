// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../components"
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko 1.0


ApplicationWindow {
    id: plEditorW

    property var roomSettings
    property var editingModel: Nheko.editPowerlevels(roomSettings.roomId)

    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    minimumWidth: 300
    minimumHeight: 400
    height: 600

    title: qsTr("Permissions in %1").arg(roomSettings.roomName);

    //    Shortcut {
    //        sequence: StandardKey.Cancel
    //        onActivated: dbb.rejected()
    //    }

    ColumnLayout {
        anchors.margins: Nheko.paddingMedium
        anchors.fill: parent
        spacing: 0


        MatrixText {
            text: qsTr("Be careful when editing permissions. You can't lower the permissions of people with a same or higher level than you. Be careful when promoting others.")
            font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.1)
            Layout.fillWidth: true
            Layout.fillHeight: false
            color: Nheko.colors.text
            Layout.bottomMargin: Nheko.paddingMedium
        }

        TabBar {
            id: bar
            width: parent.width
            palette: Nheko.colors

            component TabB : TabButton {
                id: control

                contentItem: Text {
                    text: control.text
                    font: control.font
                    opacity: enabled ? 1.0 : 0.3
                    color: control.down ? Nheko.colors.highlightedText : Nheko.colors.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    border.color: control.down ? Nheko.colors.highlight : Nheko.theme.separator
                    color: control.checked ? Nheko.colors.highlight : Nheko.colors.base
                    border.width: 1
                    radius: 2
                }
            }
            TabB {
                text: qsTr("Roles")
            }
            TabB {
                text: qsTr("Users")
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Nheko.colors.alternateBase
            border.width: 1
            border.color: Nheko.theme.separator

            StackLayout {
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium
                currentIndex: bar.currentIndex


                ColumnLayout {
                    spacing: Nheko.paddingMedium

                    MatrixText {
                        text: qsTr("Move permissions between roles to change them")
                        font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.1)
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        color: Nheko.colors.text
                    }

                    ReorderableListview {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        model: editingModel.types

                        delegate: RowLayout {
                            Column {
                                Layout.fillWidth: true

                                Text { visible: model.isType; text: model.displayName; color: Nheko.colors.text}
                                Text {
                                    visible: !model.isType;
                                    text: {
                                        if (editingModel.adminLevel == model.powerlevel)
                                        return qsTr("Administrator (%1)").arg(model.powerlevel)
                                        else if (editingModel.moderatorLevel == model.powerlevel)
                                        return qsTr("Moderator (%1)").arg(model.powerlevel)
                                        else if (editingModel.defaultUserLevel == model.powerlevel)
                                        return qsTr("User (%1)").arg(model.powerlevel)
                                        else
                                        return qsTr("Custom (%1)").arg(model.powerlevel)
                                    }
                                    color: Nheko.colors.text
                                }
                            }

                            ImageButton {
                                Layout.alignment: Qt.AlignRight
                                Layout.rightMargin: 2
                                image: model.isType ? ":/icons/icons/ui/dismiss.svg" : ":/icons/icons/ui/add-square-button.svg" 
                                visible: !model.isType || model.removeable
                                hoverEnabled: true
                                ToolTip.visible: hovered
                                ToolTip.text: model.isType ? qsTr("Remove event type") : qsTr("Add event type")
                                onClicked: {
                                    if (model.isType) {
                                        editingModel.types.remove(index);
                                    } else {
                                        typeEntry.y = offset
                                        typeEntry.visible = true
                                        typeEntry.index = index;
                                        typeEntry.forceActiveFocus()
                                    }
                                }
                            }
                        }
                        MatrixTextField {
                            id: typeEntry

                            property int index

                            width: parent.width
                            z: 5
                            visible: false

                            color: Nheko.colors.text

                            Keys.onPressed: {
                                if (typeEntry.text.includes('.') && event.matches(StandardKey.InsertParagraphSeparator)) {
                                    editingModel.types.add(typeEntry.index, typeEntry.text)
                                    typeEntry.visible = false;
                                    typeEntry.clear();
                                    event.accepted = true;
                                }
                                else if (event.matches(StandardKey.Cancel)) {
                                    typeEntry.visible = false;
                                    typeEntry.clear();
                                    event.accepted = true;
                                }
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Add new role")

                        onClicked: newPLLay.visible = true

                        Rectangle {
                            id: newPLLay

                            anchors.fill: parent
                            visible: false
                            color: Nheko.colors.alternateBase

                            RowLayout {
                                spacing: Nheko.paddingMedium
                                anchors.fill: parent

                                SpinBox {
                                    id: newPLVal

                                    Layout.fillWidth: true
                                    Layout.fillHeight: true

                                    editable: true
                                    //from: -9007199254740991
                                    //to: 9007199254740991

                                    // max qml values
                                    from: -2000000000
                                    to: 2000000000

                                    Keys.onPressed: {
                                        if (event.matches(StandardKey.InsertParagraphSeparator)) {
                                            editingModel.addRole(newPLVal.value);
                                            newPLLay.visible = false;
                                        }
                                    }
                                }

                                Button {
                                    text: qsTr("Add")
                                    Layout.preferredWidth: 100
                                    onClicked: {
                                        editingModel.addRole(newPLVal.value);
                                        newPLLay.visible = false;
                                    }
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    spacing: Nheko.paddingMedium

                    MatrixText {
                        text: qsTr("Move users up or down to change their permissions")
                        font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.1)
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                    }

                    ReorderableListview {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        model: editingModel.users

                        Column{
                            id: userEntryCompleter

                            property int index: 0

                            visible: false

                            width: parent.width
                            spacing: 1
                            z: 5
                            MatrixTextField {
                                id: userEntry

                                width: parent.width
                                //font.pixelSize: Math.ceil(quickSwitcher.textHeight * 0.6)
                                color: Nheko.colors.text
                                onTextEdited: {
                                    userCompleter.completer.searchString = text;
                                }
                                Keys.onPressed: {
                                    if (event.key == Qt.Key_Up || event.key == Qt.Key_Backtab) {
                                        event.accepted = true;
                                        userCompleter.up();
                                    } else if (event.key == Qt.Key_Down || event.key == Qt.Key_Tab) {
                                        event.accepted = true;
                                        if (event.key == Qt.Key_Tab && (event.modifiers & Qt.ShiftModifier))
                                        userCompleter.up();
                                        else
                                        userCompleter.down();
                                    } else if (event.matches(StandardKey.InsertParagraphSeparator)) {
                                        userCompleter.finishCompletion();
                                        event.accepted = true;
                                    } else if (event.matches(StandardKey.Cancel)) {
                                        typeEntry.visible = false;
                                        typeEntry.clear();
                                        event.accepted = true;
                                    }
                                }
                            }


                            Completer {
                                id: userCompleter

                                visible: userEntry.text.length > 0
                                width: parent.width
                                roomId: plEditorW.roomSettings.roomId
                                completerName: "user"
                                bottomToTop: false
                                fullWidth: true
                                avatarHeight: Nheko.avatarSize / 2
                                avatarWidth: Nheko.avatarSize / 2
                                centerRowContent: false
                                rowMargin: 2
                                rowSpacing: 2
                            }
                        }

                        Connections {
                            function onCompletionSelected(id) {
                                console.log("selected: " + id);
                                editingModel.users.add(userEntryCompleter.index, id);
                                userEntry.clear();
                                userEntryCompleter.visible = false;
                            }

                            function onCountChanged() {
                                if (userCompleter.count > 0 && (userCompleter.currentIndex < 0 || userCompleter.currentIndex >= userCompleter.count))
                                userCompleter.currentIndex = 0;

                            }

                            target: userCompleter
                        }

                        delegate: RowLayout {
                            //anchors { fill: parent; margins: 2 }
                            id: row

                            Avatar {
                                id: avatar

                                Layout.preferredHeight: Nheko.avatarSize / 2
                                Layout.preferredWidth: Nheko.avatarSize / 2
                                Layout.leftMargin: 2
                                userid: model.mxid
                                url: {
                                    if (model.isUser)
                                    return model.avatarUrl.replace("mxc://", "image://MxcImage/")
                                    else if (editingModel.adminLevel >= model.powerlevel)
                                    return "image://colorimage/:/icons/icons/ui/ribbon_star.svg?" + Nheko.colors.buttonText;
                                    else if (editingModel.moderatorLevel >= model.powerlevel)
                                    return "image://colorimage/:/icons/icons/ui/ribbon.svg?" + Nheko.colors.buttonText;
                                    else
                                    return "image://colorimage/:/icons/icons/ui/person.svg?" + Nheko.colors.buttonText;
                                }
                                displayName: model.displayName
                                enabled: false
                            }
                            Column {
                                Layout.fillWidth: true

                                Text { visible: model.isUser; text: model.displayName; color: Nheko.colors.text}
                                Text { visible: model.isUser; text: model.mxid; color: Nheko.colors.text}
                                Text {
                                    visible: !model.isUser;
                                    text: {
                                        if (editingModel.adminLevel == model.powerlevel)
                                        return qsTr("Administrator (%1)").arg(model.powerlevel)
                                        else if (editingModel.moderatorLevel == model.powerlevel)
                                        return qsTr("Moderator (%1)").arg(model.powerlevel)
                                        else
                                        return qsTr("Custom (%1)").arg(model.powerlevel)
                                    }
                                    color: Nheko.colors.text
                                }
                            }

                            ImageButton {
                                Layout.alignment: Qt.AlignRight
                                Layout.rightMargin: 2
                                image: model.isUser ? ":/icons/icons/ui/dismiss.svg" : ":/icons/icons/ui/add-square-button.svg" 
                                visible: !model.isUser || model.removeable
                                hoverEnabled: true
                                ToolTip.visible: hovered
                                ToolTip.text: model.isUser ? qsTr("Remove user") : qsTr("Add user")
                                onClicked: {
                                    if (model.isUser) {
                                        editingModel.users.remove(index);
                                    } else {
                                        userEntryCompleter.y = offset
                                        userEntryCompleter.visible = true
                                        userEntryCompleter.index = index;
                                        userEntry.forceActiveFocus()
                                    }
                                }
                            }
                        }
                    }

                }
            }
        }
    }

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        onAccepted: {
            editingModel.commit();
            plEditorW.close();
        }
        onRejected: plEditorW.close();
    }

}
