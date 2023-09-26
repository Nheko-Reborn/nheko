// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../ui"
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import im.nheko

ApplicationWindow {
    id: shortcutEditorDialog

    minimumWidth: 500
    minimumHeight: 450
    width: 500
    height: 680
    color: palette.window
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    title: qsTr("Keyboard shortcuts")

    ScrollView {
        padding: Nheko.paddingMedium
        ScrollBar.horizontal.visible: false
        anchors.fill: parent

        ListView {
            model: ShortcutRegistry

            delegate: RowLayout {
                id: del

                required property string name
                required property string description
                required property string shortcut

                spacing: Nheko.paddingMedium
                width: ListView.view.width
                height: implicitHeight + Nheko.paddingSmall * 2

                ColumnLayout {
                    spacing: Nheko.paddingSmall

                    Label {
                        text: del.name
                        font.bold: true
                        font.pointSize: fontMetrics.font.pointSize * 1.1
                    }

                    Label {
                        text: del.description
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    property bool selectingNewShortcut: false

                    text: selectingNewShortcut ? qsTr("Input..") : del.shortcut
                    onClicked: selectingNewShortcut = !selectingNewShortcut
                    Keys.onPressed: event => {
                        if (!selectingNewShortcut)
                            return;
                        event.accepted = true;

                        let keySequence = "";
                        if (event.modifiers & Qt.ControlModifier)
                            keySequence += "Ctrl+";
                        if (event.modifiers & Qt.AltModifier)
                            keySequence += "Alt+";
                        if (event.modifiers & Qt.MetaModifier)
                            keySequence += "Meta+";
                        if (event.modifiers & Qt.ShiftModifier)
                            keySequence += "Shift+";

                        if (event.key === 0 || event.key === Qt.Key_unknown || event.key === Qt.Key_Control || event.key === Qt.Key_Alt || event.key === Qt.Key_AltGr || event.key === Qt.Key_Meta || event.key === Qt.Key_Shift)
                            keySequence += "...";
                        else {
                            keySequence += ShortcutRegistry.keycodeToChar(event.key);
                            ShortcutRegistry.changeShortcut(del.name, keySequence);
                            selectingNewShortcut = false;
                        }
                    }
                }
            }
        }
    }
}
