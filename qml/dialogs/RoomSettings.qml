// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "../"
import "../ui"
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko
import im.nheko

ApplicationWindow {
    id: roomSettingsDialog

    property var roomSettings

    color: timelineRoot.palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 680
    minimumHeight: 450
    minimumWidth: 340
    modality: Qt.NonModal
    palette: timelineRoot.palette
    title: qsTr("Room Settings")
    width: 450

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok

        onAccepted: close()
    }

    Shortcut {
        sequence: StandardKey.Cancel

        onActivated: roomSettingsDialog.close()
    }
    Flickable {
        id: flickable
        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        contentHeight: contentLayout1.height
        contentWidth: roomSettingsDialog.width
        flickableDirection: Flickable.VerticalFlick

        ColumnLayout {
            id: contentLayout1
            spacing: Nheko.paddingMedium
            width: parent.width

            Avatar {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Nheko.paddingMedium
                displayName: roomSettings.roomName
                height: 130
                roomid: roomSettings.roomId
                url: roomSettings.roomAvatarUrl.replace("mxc://", "image://MxcImage/")
                width: 130

                onClicked: {
                    if (roomSettings.canChangeAvatar)
                        roomSettings.updateAvatar();
                }
            }
            Spinner {
                Layout.alignment: Qt.AlignHCenter
                foreground: timelineRoot.palette.mid
                running: roomSettings.isLoading
                visible: roomSettings.isLoading
            }
            Text {
                id: errorText
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                color: "red"
                opacity: 0
                visible: opacity > 0
                wrapMode: Text.Wrap // somehow still doesn't wrap
            }
            SequentialAnimation {
                id: hideErrorAnimation
                running: false

                PauseAnimation {
                    duration: 4000
                }
                NumberAnimation {
                    duration: 1000
                    property: 'opacity'
                    target: errorText
                    to: 0
                }
            }
            Connections {
                function onDisplayError(errorMessage) {
                    errorText.text = errorMessage;
                    errorText.opacity = 1;
                    hideErrorAnimation.restart();
                }

                target: roomSettings
            }
            TextEdit {
                id: roomName

                property bool isNameEditingAllowed: false

                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: parent.width - (Nheko.paddingSmall * 2) - nameChangeButton.anchors.leftMargin - (nameChangeButton.width * 2)
                color: timelineRoot.palette.text
                font.pixelSize: fontMetrics.font.pixelSize * 2
                horizontalAlignment: TextEdit.AlignHCenter
                readOnly: !isNameEditingAllowed
                selectByMouse: true
                text: isNameEditingAllowed ? roomSettings.plainRoomName : roomSettings.roomName
                textFormat: isNameEditingAllowed ? TextEdit.PlainText : TextEdit.RichText
                wrapMode: TextEdit.Wrap

                Keys.onPressed: {
                    if (event.matches(StandardKey.InsertLineSeparator) || event.matches(StandardKey.InsertParagraphSeparator)) {
                        roomSettings.changeName(roomName.text);
                        roomName.isNameEditingAllowed = false;
                        event.accepted = true;
                    }
                }
                Keys.onShortcutOverride: event.key === Qt.Key_Enter

                ImageButton {
                    id: nameChangeButton
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Change name of this room")
                    ToolTip.visible: hovered
                    anchors.left: roomName.right
                    anchors.leftMargin: Nheko.paddingSmall
                    anchors.verticalCenter: roomName.verticalCenter
                    hoverEnabled: true
                    image: roomName.isNameEditingAllowed ? ":/icons/icons/ui/checkmark.svg" : ":/icons/icons/ui/edit.svg"
                    visible: roomSettings.canChangeName

                    onClicked: {
                        if (roomName.isNameEditingAllowed) {
                            roomSettings.changeName(roomName.text);
                            roomName.isNameEditingAllowed = false;
                        } else {
                            roomName.isNameEditingAllowed = true;
                            roomName.focus = true;
                            roomName.selectAll();
                        }
                    }
                }
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                color: timelineRoot.palette.text
                text: qsTr("%n member(s)", "", roomSettings.memberCount)

                TapHandler {
                    onSingleTapped: TimelineManager.openRoomMembers(Rooms.getRoomById(roomSettings.roomId))
                }
                NhekoCursorShape {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                }
            }
            TextArea {
                id: roomTopic

                property bool cut: implicitHeight > 100
                property bool isTopicEditingAllowed: false
                property bool showMore: false

                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                Layout.leftMargin: Nheko.paddingLarge
                Layout.maximumHeight: showMore ? Number.POSITIVE_INFINITY : 100
                Layout.preferredHeight: implicitHeight
                Layout.rightMargin: Nheko.paddingLarge
                background: null
                clip: true
                color: timelineRoot.palette.text
                horizontalAlignment: TextEdit.AlignHCenter
                readOnly: !isTopicEditingAllowed
                selectByMouse: !Settings.mobileMode
                text: isTopicEditingAllowed ? roomSettings.plainRoomTopic : roomSettings.roomTopic
                textFormat: isTopicEditingAllowed ? TextEdit.PlainText : TextEdit.RichText
                wrapMode: TextEdit.WordWrap

                onLinkActivated: Nheko.openLink(link)

                NhekoCursorShape {
                    anchors.fill: parent
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }
            ImageButton {
                id: topicChangeButton
                Layout.alignment: Qt.AlignHCenter
                ToolTip.delay: Nheko.tooltipDelay
                ToolTip.text: qsTr("Change topic of this room")
                ToolTip.visible: hovered
                hoverEnabled: true
                image: roomTopic.isTopicEditingAllowed ? ":/icons/icons/ui/checkmark.svg" : ":/icons/icons/ui/edit.svg"
                visible: roomSettings.canChangeTopic

                onClicked: {
                    if (roomTopic.isTopicEditingAllowed) {
                        roomSettings.changeTopic(roomTopic.text);
                        roomTopic.isTopicEditingAllowed = false;
                    } else {
                        roomTopic.isTopicEditingAllowed = true;
                        roomTopic.showMore = true;
                        roomTopic.focus = true;
                        //roomTopic.selectAll();
                    }
                }
            }
            Item {
                id: showMorePlaceholder
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredHeight: showMoreButton.height
                Layout.preferredWidth: showMoreButton.width
                visible: roomTopic.cut
            }
            GridLayout {
                Layout.fillWidth: true
                Layout.margins: Nheko.paddingMedium
                columns: 2
                rowSpacing: Nheko.paddingMedium

                Label {
                    color: timelineRoot.palette.text
                    font.bold: true
                    text: qsTr("SETTINGS")
                }
                Item {
                    Layout.fillWidth: true
                }
                Label {
                    Layout.fillWidth: true
                    color: timelineRoot.palette.text
                    text: qsTr("Notifications")
                }
                ComboBox {
                    Layout.fillWidth: true
                    currentIndex: roomSettings.notifications
                    model: [qsTr("Muted"), qsTr("Mentions only"), qsTr("All messages")]

                    onActivated: {
                        roomSettings.changeNotifications(index);
                    }

                    WheelHandler {
                    } // suppress scrolling changing values
                }
                Label {
                    Layout.fillWidth: true
                    color: timelineRoot.palette.text
                    text: qsTr("Room access")
                }
                ComboBox {
                    Layout.fillWidth: true
                    currentIndex: roomSettings.accessJoinRules
                    enabled: roomSettings.canChangeJoinRules
                    model: {
                        let opts = [qsTr("Anyone and guests"), qsTr("Anyone"), qsTr("Invited users")];
                        if (roomSettings.supportsKnocking)
                            opts.push(qsTr("By knocking"));
                        if (roomSettings.supportsRestricted)
                            opts.push(qsTr("Restricted by membership in other rooms"));
                        return opts;
                    }

                    onActivated: {
                        roomSettings.changeAccessRules(index);
                    }

                    WheelHandler {
                    } // suppress scrolling changing values
                }
                Label {
                    color: timelineRoot.palette.text
                    text: qsTr("Encryption")
                }
                ToggleButton {
                    id: encryptionToggle
                    Layout.alignment: Qt.AlignRight
                    checked: roomSettings.isEncryptionEnabled

                    onCheckedChanged: {
                        if (roomSettings.isEncryptionEnabled) {
                            checked = true;
                            return;
                        }
                        confirmEncryptionDialog.open();
                    }
                }
                Platform.MessageDialog {
                    id: confirmEncryptionDialog
                    buttons: Platform.MessageDialog.Ok | Platform.MessageDialog.Cancel
                    modality: Qt.NonModal
                    text: qsTr("Encryption is currently experimental and things might break unexpectedly. <br>
                                Please take note that it can't be disabled afterwards.")
                    title: qsTr("End-to-End Encryption")

                    onAccepted: {
                        if (roomSettings.isEncryptionEnabled)
                            return;
                        roomSettings.enableEncryption();
                    }
                    onRejected: {
                        encryptionToggle.checked = false;
                    }
                }
                Label {
                    color: timelineRoot.palette.text
                    text: qsTr("Sticker & Emote Settings")
                }
                Button {
                    Layout.alignment: Qt.AlignRight
                    ToolTip.text: qsTr("Change what packs are enabled, remove packs or create new ones")
                    text: qsTr("Change")

                    onClicked: TimelineManager.openImagePackSettings(roomSettings.roomId)
                }
                Label {
                    color: timelineRoot.palette.text
                    text: qsTr("Hidden events")
                }
                HiddenEventsDialog {
                    id: hiddenEventsDialog
                    roomName: roomSettings.roomName
                    roomid: roomSettings.roomId
                }
                Button {
                    Layout.alignment: Qt.AlignRight
                    ToolTip.text: qsTr("Select events to hide in this room")
                    text: qsTr("Configure")

                    onClicked: hiddenEventsDialog.show()
                }
                Item {
                    // for adding extra space between sections
                    Layout.fillWidth: true
                }
                Item {
                    // for adding extra space between sections
                    Layout.fillWidth: true
                }
                Label {
                    color: timelineRoot.palette.text
                    font.bold: true
                    text: qsTr("INFO")
                }
                Item {
                    Layout.fillWidth: true
                }
                Label {
                    color: timelineRoot.palette.text
                    text: qsTr("Internal ID")
                }
                AbstractButton {
                    // AbstractButton does not allow setting text color
                    Layout.alignment: Qt.AlignRight
                    Layout.fillWidth: true
                    Layout.preferredHeight: idLabel.height

                    onClicked: {
                        textEdit.selectAll();
                        textEdit.copy();
                        toolTipTimer.start();
                    }

                    Label {
                        // TextEdit does not trigger onClicked
                        id: idLabel
                        ToolTip.text: qsTr("Copied to clipboard")
                        ToolTip.visible: toolTipTimer.running
                        color: timelineRoot.palette.text
                        font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 0.8)
                        horizontalAlignment: Text.AlignRight
                        text: roomSettings.roomId
                        width: parent.width
                        wrapMode: Text.WrapAnywhere
                    }
                    TextEdit {
                        // label does not allow selection
                        id: textEdit
                        text: roomSettings.roomId
                        visible: false
                    }
                    Timer {
                        id: toolTipTimer
                    }
                }
                Label {
                    color: timelineRoot.palette.text
                    text: qsTr("Room Version")
                }
                Label {
                    Layout.alignment: Qt.AlignRight
                    color: timelineRoot.palette.text
                    font.pixelSize: fontMetrics.font.pixelSize
                    text: roomSettings.roomVersion
                }
            }
        }
    }
    Button {
        id: showMoreButton
        anchors.horizontalCenter: flickable.horizontalCenter
        text: roomTopic.showMore ? qsTr("show less") : qsTr("show more")
        visible: roomTopic.cut
        y: Math.min(showMorePlaceholder.y + contentLayout1.y - flickable.contentY, flickable.height - height)

        onClicked: {
            roomTopic.showMore = !roomTopic.showMore;
            console.log(flickable.visibleArea);
        }
    }
}
