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
            model: KeySequenceRegistry

            delegate: RowLayout {
                id: del

                required property string name
                required property string keySequence

                spacing: Nheko.paddingMedium
                width: ListView.view.width
                height: implicitHeight + Nheko.paddingSmall * 2

                Label {
                    text: del.name
                }

                Item { Layout.fillWidth: true }

                Button {
                    property bool selectingNewKeySequence: false

                    text: selectingNewKeySequence ? qsTr("Input..") : (del.keySequence === "" ? "None" : del.keySequence)
                    onClicked: selectingNewKeySequence = !selectingNewKeySequence
                    Keys.onPressed: event => {
                        if (!selectingNewKeySequence)
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

                        if (event.key === 0 || event.key === Qt.Key_unknown || event.key === Qt.Key_Control || event.key === Qt.Key_Alt ||
                            event.key === Qt.Key_AltGr || event.key === Qt.Key_Meta || event.key === Qt.Key_Super_L || event.key === Qt.Key_Super_R ||
                            event.key === Qt.Key_Shift)
                            keySequence += "...";
                        else {
                            keySequence += KeySequenceRegistry.keycodeToChar(event.key);
                            KeySequenceRegistry.changeKeySequence(del.name, keySequence);
                            selectingNewKeySequence = false;
                        }
                    }
                }
            }
        }
    }
}
