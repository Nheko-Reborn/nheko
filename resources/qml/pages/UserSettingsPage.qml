// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound
import ".."
import "../ui"
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import QtQuick.Window 2.15
import im.nheko 1.0

Rectangle {
    id: userSettingsDialog

    property int collapsePoint: 600
    property bool collapsed: width < collapsePoint
    color: palette.window

    ScrollView {
        id: scroll

        ScrollBar.horizontal.visible: false
        anchors.fill: parent
        anchors.topMargin: (collapsed? backButton.height : 0)+Nheko.paddingLarge
        leftPadding: collapsed? Nheko.paddingMedium : Nheko.paddingLarge
        bottomPadding: Nheko.paddingLarge
        contentWidth: availableWidth

        ColumnLayout {
            id: grid

            spacing: Nheko.paddingMedium

            width: scroll.availableWidth
            anchors.fill: parent
            anchors.leftMargin: userSettingsDialog.collapsed ? 0 : (userSettingsDialog.width-userSettingsDialog.collapsePoint) * 0.4 + Nheko.paddingLarge
            anchors.rightMargin: anchors.leftMargin


            Repeater {
                model: UserSettingsModel

                delegate: GridLayout {
                    width: scroll.availableWidth
                    columns: collapsed? 1 : 2
                    rows: collapsed? 2: 1
                    required property var model
                    id: r

                    Label {
                        Layout.alignment: Qt.AlignLeft
                        Layout.fillWidth: true
                        color: palette.text
                        text: model.name
                        //Layout.column: 0
                        Layout.columnSpan: (model.type == UserSettingsModel.SectionTitle && !userSettingsDialog.collapsed) ? 2 : 1
                        //Layout.row: model.index
                        //Layout.minimumWidth: implicitWidth
                        Layout.leftMargin: model.type == UserSettingsModel.SectionTitle ? 0 : Nheko.paddingMedium
                        Layout.topMargin: model.type == UserSettingsModel.SectionTitle ? Nheko.paddingLarge : 0
                        font.pointSize: 1.1 * fontMetrics.font.pointSize

                        HoverHandler {
                            id: hovered
                            enabled: model.description ?? false
                        }
                        ToolTip.visible: hovered.hovered && model.description
                        ToolTip.text: model.description ?? ""
                        ToolTip.delay: Nheko.tooltipDelay
                        wrapMode: Text.Wrap
                    }

                    DelegateChooser {
                        id: chooser

                        roleValue: model.type
                        Layout.alignment: Qt.AlignRight

                        Layout.columnSpan: (model.type == UserSettingsModel.SectionTitle && !userSettingsDialog.collapsed) ? 2 : 1
                        Layout.preferredHeight: child.height
                        Layout.preferredWidth: child.implicitWidth
                        Layout.maximumWidth: model.type == UserSettingsModel.SectionTitle ? Number.POSITIVE_INFINITY : 400
                        Layout.fillWidth: model.type == UserSettingsModel.SectionTitle || model.type == UserSettingsModel.Options || model.type == UserSettingsModel.Number
                        Layout.rightMargin: model.type == UserSettingsModel.SectionTitle ? 0 : Nheko.paddingMedium

                        DelegateChoice {
                            roleValue: UserSettingsModel.Toggle
                            ToggleButton {
                                checked: model.value
                                onCheckedChanged: model.value = checked
                                enabled: model.enabled
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.Options
                            ComboBox {
                                anchors.right: parent.right
                                model: r.model.values
                                currentIndex: r.model.value
                                width: Math.min(implicitWidth, scroll.availableWidth - Nheko.paddingMedium)
                                onCurrentIndexChanged: r.model.value = currentIndex
                                implicitContentWidthPolicy: ComboBox.WidestTextWhenCompleted

                                WheelHandler{} // suppress scrolling changing values
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.Integer

                            SpinBox {
                                anchors.right: parent.right
                                from: model.valueLowerBound
                                to: model.valueUpperBound
                                stepSize: model.valueStep
                                value: model.value
                                onValueChanged: model.value = value
                                editable: true

                                WheelHandler{} // suppress scrolling changing values
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.Double

                            SpinBox {
                                id: spinbox

                                readonly property double div: 100
                                readonly property int decimals: 2

                                anchors.right: parent.right
                                from: model.valueLowerBound * div
                                to: model.valueUpperBound * div
                                stepSize: model.valueStep * div
                                value: model.value * div
                                onValueChanged: model.value = value/div
                                editable: true

                                property real realValue: value / div

                                validator: DoubleValidator {
                                    bottom: Math.min(spinbox.from/spinbox.div, spinbox.to/spinbox.div)
                                    top:  Math.max(spinbox.from/spinbox.div, spinbox.to/spinbox.div)
                                }

                                textFromValue: function(value, locale) {
                                    return Number(value / spinbox.div).toLocaleString(locale, 'f', spinbox.decimals)
                                }

                                valueFromText: function(text, locale) {
                                    return Number.fromLocaleString(locale, text) * spinbox.div
                                }

                                WheelHandler{} // suppress scrolling changing values
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.ReadOnlyText
                            TextEdit {
                                color: palette.text
                                text: model.value
                                readOnly: true
                                selectByMouse: !Settings.mobileMode
                                textFormat: Text.PlainText
                            }
                        }
                        DelegateChoice {
                            roleValue: UserSettingsModel.SectionTitle
                            Item {
                                width: grid.width
                                height: fontMetrics.lineSpacing
                                Rectangle {
                                    anchors.topMargin: Nheko.paddingSmall
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.right: parent.right
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

                                    HiddenEventsDialog {}
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
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: Nheko.paddingMedium
        width: Nheko.avatarSize
        height: Nheko.avatarSize
        image: ":/icons/icons/ui/angle-arrow-left.svg"
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Back")
        onClicked: mainWindow.pop()
    }

}

