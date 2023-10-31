// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound
import ".."
import "../dialogs"
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import im.nheko

Rectangle {
    id: userSettingsDialog

    property int collapsePoint: 600
    property bool collapsed: width < collapsePoint

    color: palette.window

    ScrollView {
        id: scroll

        ScrollBar.horizontal.visible: false
        anchors.fill: parent
        anchors.topMargin: (collapsed ? backButton.height : 0) + Nheko.paddingLarge
        bottomPadding: Nheko.paddingLarge
        contentWidth: availableWidth
        leftPadding: collapsed ? Nheko.paddingMedium : Nheko.paddingLarge

        ColumnLayout {
            id: grid

            anchors.fill: parent
            anchors.leftMargin: userSettingsDialog.collapsed ? 0 : (userSettingsDialog.width - userSettingsDialog.collapsePoint) * 0.4 + Nheko.paddingLarge
            anchors.rightMargin: anchors.leftMargin
            spacing: Nheko.paddingMedium
            width: scroll.availableWidth

            Repeater {
                model: UserSettingsModel

                delegate: GridLayout {
                    id: r

                    required property var model

                    columns: collapsed ? 1 : 2
                    rows: collapsed ? 2 : 1
                    width: scroll.availableWidth

                    Label {
                        Layout.alignment: Qt.AlignLeft
                        //Layout.column: 0
                        Layout.columnSpan: (model.type == UserSettingsModel.SectionTitle && !userSettingsDialog.collapsed) ? 2 : 1
                        Layout.fillWidth: true
                        //Layout.row: model.index
                        //Layout.minimumWidth: implicitWidth
                        Layout.leftMargin: model.type == UserSettingsModel.SectionTitle ? 0 : Nheko.paddingMedium
                        Layout.topMargin: model.type == UserSettingsModel.SectionTitle ? Nheko.paddingLarge : 0
                        ToolTip.delay: Nheko.tooltipDelay
                        ToolTip.text: model.description ?? ""
                        ToolTip.visible: hovered.hovered && model.description
                        color: palette.text
                        font.pointSize: 1.1 * fontMetrics.font.pointSize
                        text: model.name
                        wrapMode: Text.Wrap

                        HoverHandler {
                            id: hovered

                            enabled: model.description ?? false
                        }
                    }
                    DelegateChooser {
                        id: chooser

                        Layout.alignment: Qt.AlignRight
                        Layout.columnSpan: (model.type == UserSettingsModel.SectionTitle && !userSettingsDialog.collapsed) ? 2 : 1
                        Layout.fillWidth: model.type == UserSettingsModel.SectionTitle || model.type == UserSettingsModel.Options || model.type == UserSettingsModel.Number
                        Layout.maximumWidth: model.type == UserSettingsModel.SectionTitle ? Number.POSITIVE_INFINITY : 400
                        Layout.preferredHeight: child.height
                        Layout.preferredWidth: child.implicitWidth
                        Layout.rightMargin: model.type == UserSettingsModel.SectionTitle ? 0 : Nheko.paddingMedium
                        roleValue: model.type

                        DelegateChoice {
                            roleValue: UserSettingsModel.Toggle

                            ToggleButton {
                                checked: model.value
                                enabled: model.enabled

                                onCheckedChanged: model.value = checked
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.Options

                            ComboBox {
                                anchors.right: parent.right
                                currentIndex: r.model.value
                                implicitContentWidthPolicy: ComboBox.WidestTextWhenCompleted
                                model: r.model.values
                                width: Math.min(implicitWidth, scroll.availableWidth - Nheko.paddingMedium)

                                onCurrentIndexChanged: r.model.value = currentIndex

                                WheelHandler {
                                } // suppress scrolling changing values
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.Integer

                            SpinBox {
                                anchors.right: parent.right
                                editable: true
                                from: model.valueLowerBound
                                stepSize: model.valueStep
                                to: model.valueUpperBound
                                value: model.value

                                onValueChanged: model.value = value

                                WheelHandler {
                                } // suppress scrolling changing values
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.Double

                            SpinBox {
                                id: spinbox

                                readonly property int decimals: 2
                                readonly property double div: 100
                                property real realValue: value / div

                                anchors.right: parent.right
                                editable: true
                                from: model.valueLowerBound * div
                                stepSize: model.valueStep * div
                                textFromValue: function (value, locale) {
                                    return Number(value / spinbox.div).toLocaleString(locale, 'f', spinbox.decimals);
                                }
                                to: model.valueUpperBound * div
                                value: model.value * div
                                valueFromText: function (text, locale) {
                                    return Number.fromLocaleString(locale, text) * spinbox.div;
                                }

                                validator: DoubleValidator {
                                    bottom: Math.min(spinbox.from / spinbox.div, spinbox.to / spinbox.div)
                                    top: Math.max(spinbox.from / spinbox.div, spinbox.to / spinbox.div)
                                }

                                onValueChanged: model.value = value / div

                                WheelHandler {
                                } // suppress scrolling changing values
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.ReadOnlyText

                            TextEdit {
                                color: palette.text
                                readOnly: true
                                text: model.value
                                textFormat: Text.PlainText
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.SectionTitle

                            Item {
                                height: fontMetrics.lineSpacing
                                width: grid.width

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.topMargin: Nheko.paddingSmall
                                    color: palette.buttonText
                                    height: 1
                                }
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.KeyStatus

                            Text {
                                color: model.good ? "green" : Nheko.theme.error
                                text: model.value ? qsTr("CACHED") : qsTr("NOT CACHED")
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.SessionKeyImportExport

                            RowLayout {
                                Button {
                                    text: qsTr("IMPORT")

                                    onClicked: UserSettingsModel.importSessionKeys()
                                }
                                Button {
                                    text: qsTr("EXPORT")

                                    onClicked: UserSettingsModel.exportSessionKeys()
                                }
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.XSignKeysRequestDownload

                            RowLayout {
                                Button {
                                    text: qsTr("DOWNLOAD")

                                    onClicked: UserSettingsModel.downloadCrossSigningSecrets()
                                }
                                Button {
                                    text: qsTr("REQUEST")

                                    onClicked: UserSettingsModel.requestCrossSigningSecrets()
                                }
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.ConfigureHiddenEvents

                            Button {
                                text: qsTr("CONFIGURE")

                                onClicked: {
                                    var dialog = hiddenEventsDialog.createObject();
                                    dialog.show();
                                    destroyOnClose(dialog);
                                }

                                Component {
                                    id: hiddenEventsDialog

                                    HiddenEventsDialog {
                                    }
                                }
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.ManageIgnoredUsers

                            Button {
                                text: qsTr("MANAGE")

                                onClicked: {
                                    var dialog = ignoredUsersDialog.createObject();
                                    dialog.show();
                                    destroyOnClose(dialog);
                                }

                                Component {
                                    id: ignoredUsersDialog

                                    IgnoredUsers {
                                    }
                                }
                            }
                        }
                        DelegateChoice {
                            Text {
                                text: model.value
                            }
                        }
                    }
                }
            }
        }
    }
    ImageButton {
        id: backButton

        ToolTip.text: qsTr("Back")
        ToolTip.visible: hovered
        anchors.left: parent.left
        anchors.margins: Nheko.paddingMedium
        anchors.top: parent.top
        height: Nheko.avatarSize
        image: ":/icons/icons/ui/angle-arrow-left.svg"
        width: Nheko.avatarSize

        onClicked: mainWindow.pop()
    }
}
