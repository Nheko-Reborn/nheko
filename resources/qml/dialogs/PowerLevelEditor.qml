// SPDX-FileCopyrightText: Nheko Contributors
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

    property var editingModel: Nheko.editPowerlevels(roomSettings.roomId)
    property var roomSettings

    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 600
    minimumHeight: 400
    minimumWidth: 300
    modality: Qt.NonModal
    title: qsTr("Permissions in %1").arg(roomSettings.roomName)
    width: 300

    footer: DialogButtonBox {
        id: dbb

        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onAccepted: {
            if (editingModel.isSpace) {
                // TODO(Nico): Replace with showing a list of spaces to apply to
                editingModel.updateSpacesModel();
                plEditorW.close();
                timelineRoot.showSpacePLApplyPrompt(roomSettings, editingModel);
            } else {
                editingModel.commit();
                plEditorW.close();
            }
        }
        onRejected: plEditorW.close()
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
            text: qsTr("Be careful when editing permissions. You can't lower the permissions of people with a same or higher level than you. Be careful when promoting others.")
        }
        TabBar {
            id: bar

            Layout.preferredWidth: parent.width

            NhekoTabButton {
                text: qsTr("Roles")
            }
            NhekoTabButton {
                text: qsTr("Users")
            }
        }
        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            border.color: Nheko.theme.separator
            border.width: 1
            color: palette.alternateBase

            StackLayout {
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium
                currentIndex: bar.currentIndex

                ColumnLayout {
                    spacing: Nheko.paddingMedium

                    MatrixText {
                        Layout.fillHeight: false
                        Layout.fillWidth: true
                        color: palette.text
                        font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.1)
                        text: qsTr("Move permissions between roles to change them")
                    }
                    ReorderableListview {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        model: editingModel.types

                        delegate: RowLayout {
                            Column {
                                Layout.fillWidth: true

                                Text {
                                    color: palette.text
                                    text: model.displayName
                                    visible: model.isType
                                }
                                Text {
                                    color: palette.text
                                    text: {
                                        if (editingModel.adminLevel == model.powerlevel)
                                            return qsTr("Administrator (%1)").arg(model.powerlevel);
                                        else if (editingModel.moderatorLevel == model.powerlevel)
                                            return qsTr("Moderator (%1)").arg(model.powerlevel);
                                        else if (editingModel.defaultUserLevel == model.powerlevel)
                                            return qsTr("User (%1)").arg(model.powerlevel);
                                        else
                                            return qsTr("Custom (%1)").arg(model.powerlevel);
                                    }
                                    visible: !model.isType
                                }
                            }
                            ImageButton {
                                Layout.alignment: Qt.AlignRight
                                Layout.rightMargin: 2
                                ToolTip.text: model.isType ? qsTr("Remove event type") : qsTr("Add event type")
                                ToolTip.visible: hovered
                                hoverEnabled: true
                                image: model.isType ? ":/icons/icons/ui/dismiss.svg" : ":/icons/icons/ui/add-square-button.svg"
                                visible: !model.isType || model.removeable

                                onClicked: {
                                    if (model.isType) {
                                        editingModel.types.remove(index);
                                    } else {
                                        typeEntry.y = offset;
                                        typeEntry.visible = true;
                                        typeEntry.index = index;
                                        typeEntry.forceActiveFocus();
                                    }
                                }
                            }
                        }

                        MatrixTextField {
                            id: typeEntry

                            property int index

                            color: palette.text
                            visible: false
                            width: parent.width
                            z: 5

                            Keys.onPressed: {
                                if (typeEntry.text.includes('.') && event.matches(StandardKey.InsertParagraphSeparator)) {
                                    editingModel.types.add(typeEntry.index, typeEntry.text);
                                    typeEntry.visible = false;
                                    typeEntry.clear();
                                    event.accepted = true;
                                } else if (event.matches(StandardKey.Cancel)) {
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
                            color: palette.alternateBase
                            visible: false

                            RowLayout {
                                anchors.fill: parent
                                spacing: Nheko.paddingMedium

                                SpinBox {
                                    id: newPLVal

                                    Layout.fillHeight: true
                                    Layout.fillWidth: true
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
                                    Layout.preferredWidth: 100
                                    text: qsTr("Add")

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
                        Layout.fillHeight: false
                        Layout.fillWidth: true
                        font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 1.1)
                        text: qsTr("Move users up or down to change their permissions")
                    }
                    ReorderableListview {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        model: editingModel.users

                        delegate: RowLayout {
                            //anchors { fill: parent; margins: 2 }
                            id: row

                            Avatar {
                                id: avatar

                                Layout.leftMargin: 2
                                Layout.preferredHeight: Nheko.avatarSize / 2
                                Layout.preferredWidth: Nheko.avatarSize / 2
                                displayName: model.displayName
                                enabled: false
                                url: {
                                    if (model.isUser)
                                        return model.avatarUrl.replace("mxc://", "image://MxcImage/");
                                    else if (editingModel.adminLevel >= model.powerlevel)
                                        return "image://colorimage/:/icons/icons/ui/ribbon_star.svg?" + palette.buttonText;
                                    else if (editingModel.moderatorLevel >= model.powerlevel)
                                        return "image://colorimage/:/icons/icons/ui/ribbon.svg?" + palette.buttonText;
                                    else
                                        return "image://colorimage/:/icons/icons/ui/person.svg?" + palette.buttonText;
                                }
                                userid: model.mxid
                            }
                            Column {
                                Layout.fillWidth: true

                                Text {
                                    color: palette.text
                                    text: model.displayName
                                    visible: model.isUser
                                }
                                Text {
                                    color: palette.text
                                    text: model.mxid
                                    visible: model.isUser
                                }
                                Text {
                                    color: palette.text
                                    text: {
                                        if (editingModel.adminLevel == model.powerlevel)
                                            return qsTr("Administrator (%1)").arg(model.powerlevel);
                                        else if (editingModel.moderatorLevel == model.powerlevel)
                                            return qsTr("Moderator (%1)").arg(model.powerlevel);
                                        else
                                            return qsTr("Custom (%1)").arg(model.powerlevel);
                                    }
                                    visible: !model.isUser
                                }
                            }
                            ImageButton {
                                Layout.alignment: Qt.AlignRight
                                Layout.rightMargin: 2
                                ToolTip.text: model.isUser ? qsTr("Remove user") : qsTr("Add user")
                                ToolTip.visible: hovered
                                hoverEnabled: true
                                image: model.isUser ? ":/icons/icons/ui/dismiss.svg" : ":/icons/icons/ui/add-square-button.svg"
                                visible: !model.isUser || model.removeable

                                onClicked: {
                                    if (model.isUser) {
                                        editingModel.users.remove(index);
                                    } else {
                                        userEntryCompleter.y = offset;
                                        userEntryCompleter.visible = true;
                                        userEntryCompleter.index = index;
                                        userEntry.forceActiveFocus();
                                    }
                                }
                            }
                        }

                        Column {
                            id: userEntryCompleter

                            property int index: 0

                            spacing: 1
                            visible: false
                            width: parent.width
                            z: 5

                            MatrixTextField {
                                id: userEntry

                                //font.pixelSize: Math.ceil(quickSwitcher.textHeight * 0.6)
                                color: palette.text
                                width: parent.width

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
                                        if (userCompleter.currentCompletion()) {
                                            userCompleter.finishCompletion();
                                        } else if (userEntry.text.startsWith("@") && userEntry.text.includes(":")) {
                                            userCompletionConnections.onCompletionSelected(userEntry.text);
                                        }
                                        event.accepted = true;
                                    } else if (event.matches(StandardKey.Cancel)) {
                                        typeEntry.visible = false;
                                        typeEntry.clear();
                                        event.accepted = true;
                                    }
                                }
                                onTextEdited: {
                                    userCompleter.completer.searchString = text;
                                }
                            }
                            Completer {
                                id: userCompleter

                                avatarHeight: Nheko.avatarSize / 2
                                avatarWidth: Nheko.avatarSize / 2
                                bottomToTop: false
                                centerRowContent: false
                                completerName: "user"
                                fullWidth: true
                                roomId: plEditorW.roomSettings.roomId
                                rowMargin: 2
                                rowSpacing: 2
                                visible: userEntry.text.length > 0
                                width: parent.width
                            }
                        }
                        Connections {
                            id: userCompletionConnections

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
                    }
                }
            }
        }
    }
}
