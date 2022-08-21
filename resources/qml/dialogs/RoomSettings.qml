// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import "../ui"
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0

ApplicationWindow {
    id: roomSettingsDialog

    property var roomSettings

    minimumWidth: 340
    minimumHeight: 450
    width: 450
    height: 680
    palette: Nheko.colors
    color: Nheko.colors.window
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    title: qsTr("Room Settings")

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: roomSettingsDialog.close()
    }
    ScrollHelper {
        flickable: flickable
        anchors.fill: flickable
    }
    Flickable {
        id: flickable
        boundsBehavior: Flickable.StopAtBounds
        anchors.fill: parent
        clip: true
        flickableDirection: Flickable.VerticalFlick
        contentWidth: roomSettingsDialog.width
        contentHeight: contentLayout1.height
        ColumnLayout {
            id: contentLayout1
            width: parent.width
            spacing: Nheko.paddingMedium

            Avatar {
                id: displayAvatar

                Layout.topMargin: Nheko.paddingMedium
                url: roomSettings.roomAvatarUrl.replace("mxc://", "image://MxcImage/")
                roomid: roomSettings.roomId
                displayName: roomSettings.roomName
                height: 130
                width: 130
                Layout.alignment: Qt.AlignHCenter
                onClicked: TimelineManager.openImageOverlay(null, roomSettings.roomAvatarUrl, "", 0, 0)

                ImageButton {
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Change room avatar.")
                    anchors.left: displayAvatar.left
                    anchors.top: displayAvatar.top
                    anchors.leftMargin: Nheko.paddingMedium
                    anchors.topMargin: Nheko.paddingMedium
                    visible: roomSettings.canChangeAvatar
                    image: ":/icons/icons/ui/edit.svg"
                    onClicked: {
                        roomSettings.updateAvatar();
                    }

                }
            }

            Spinner {
                Layout.alignment: Qt.AlignHCenter
                visible: roomSettings.isLoading
                foreground: Nheko.colors.mid
                running: roomSettings.isLoading
            }

            Text {
                id: errorText

                color: "red"
                visible: opacity > 0
                opacity: 0
                Layout.alignment: Qt.AlignHCenter
                wrapMode: Text.Wrap // somehow still doesn't wrap
                Layout.fillWidth: true
            }

            SequentialAnimation {
                id: hideErrorAnimation

                running: false

                PauseAnimation {
                    duration: 4000
                }

                NumberAnimation {
                    target: errorText
                    property: 'opacity'
                    to: 0
                    duration: 1000
                }

            }

            Connections {
                target: roomSettings
                function onDisplayError(errorMessage) {
                    errorText.text = errorMessage;
                    errorText.opacity = 1;
                    hideErrorAnimation.restart();
                }
            }

            TextEdit {
                id: roomName

                property bool isNameEditingAllowed: false

                readOnly: !isNameEditingAllowed
                textFormat: isNameEditingAllowed ? TextEdit.PlainText : TextEdit.RichText
                text: isNameEditingAllowed ? roomSettings.plainRoomName : roomSettings.roomName
                font.pixelSize: fontMetrics.font.pixelSize * 2
                color: Nheko.colors.text

                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: parent.width - (Nheko.paddingSmall * 2) - nameChangeButton.anchors.leftMargin - (nameChangeButton.width * 2)
                horizontalAlignment: TextEdit.AlignHCenter
                wrapMode: TextEdit.Wrap
                selectByMouse: true

                Keys.onShortcutOverride: event.key === Qt.Key_Enter
                Keys.onPressed: {
                    if (event.matches(StandardKey.InsertLineSeparator) || event.matches(StandardKey.InsertParagraphSeparator)) {
                        roomSettings.changeName(roomName.text);
                        roomName.isNameEditingAllowed = false;
                        event.accepted = true;
                    }
                }

                ImageButton {
                    id: nameChangeButton
                    visible: roomSettings.canChangeName
                    anchors.leftMargin: Nheko.paddingSmall
                    anchors.left: roomName.right
                    anchors.verticalCenter: roomName.verticalCenter
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Change name of this room")
                    ToolTip.delay: Nheko.tooltipDelay
                    image: roomName.isNameEditingAllowed ? ":/icons/icons/ui/checkmark.svg" : ":/icons/icons/ui/edit.svg"
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

            RowLayout {
                spacing: Nheko.paddingMedium
                Layout.alignment: Qt.AlignHCenter

                Label {
                    text: qsTr("%n member(s)", "", roomSettings.memberCount)
                    color: Nheko.colors.text
                }

                ImageButton {
                    image: ":/icons/icons/ui/people.svg"
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("View members of %1").arg(roomSettings.roomName)
                    onClicked: TimelineManager.openRoomMembers(Rooms.getRoomById(roomSettings.roomId))
                }

            }

            TextArea {
                id: roomTopic
                property bool cut: implicitHeight > 100
                property bool showMore: false
                clip: true
                Layout.maximumHeight: showMore? Number.POSITIVE_INFINITY : 100
                Layout.preferredHeight: implicitHeight
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                Layout.leftMargin: Nheko.paddingLarge
                Layout.rightMargin: Nheko.paddingLarge

                property bool isTopicEditingAllowed: false

                readOnly: !isTopicEditingAllowed
                textFormat: isTopicEditingAllowed ? TextEdit.PlainText : TextEdit.RichText
                text: isTopicEditingAllowed
                        ? roomSettings.plainRoomTopic
                        : (roomSettings.plainRoomTopic === "" ? ("<i>" + qsTr("No topic set") + "</i>") : roomSettings.roomTopic)
                wrapMode: TextEdit.WordWrap
                background: null
                selectByMouse: !Settings.mobileMode
                color: Nheko.colors.text
                horizontalAlignment: TextEdit.AlignHCenter
                onLinkActivated: Nheko.openLink(link)

                CursorShape {
                    anchors.fill: parent
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }

            }

            ImageButton {
                id: topicChangeButton
                Layout.alignment: Qt.AlignHCenter
                visible: roomSettings.canChangeTopic
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Change topic of this room")
                ToolTip.delay: Nheko.tooltipDelay
                image: roomTopic.isTopicEditingAllowed ? ":/icons/icons/ui/checkmark.svg" : ":/icons/icons/ui/edit.svg"
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
                Layout.alignment: Qt.AlignHCenter
                id: showMorePlaceholder
                Layout.preferredHeight: showMoreButton.height
                Layout.preferredWidth: showMoreButton.width
                visible: roomTopic.cut
            }

            GridLayout {
                columns: 2
                rowSpacing: Nheko.paddingMedium
                Layout.margins: Nheko.paddingMedium
                Layout.fillWidth: true

                Label {
                    text: qsTr("SETTINGS")
                    font.bold: true
                    color: Nheko.colors.text
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Notifications")
                    Layout.fillWidth: true
                    color: Nheko.colors.text
                }

                ComboBox {
                    model: [qsTr("Muted"), qsTr("Mentions only"), qsTr("All messages")]
                    currentIndex: roomSettings.notifications
                    onActivated: {
                        roomSettings.changeNotifications(index);
                    }
                    Layout.fillWidth: true
                    WheelHandler{} // suppress scrolling changing values
                }

                Label {
                    text: qsTr("Room access")
                    Layout.fillWidth: true
                    color: Nheko.colors.text
                }

                ComboBox {
                    enabled: roomSettings.canChangeJoinRules
                    model: {
                        let opts = [qsTr("Anyone and guests"), qsTr("Anyone"), qsTr("Invited users")];
                        if (roomSettings.supportsKnocking)
                            opts.push(qsTr("By knocking"));

                        if (roomSettings.supportsRestricted)
                            opts.push(qsTr("Restricted by membership in other rooms"));

                        if (roomSettings.supportsKnockRestricted)
                            opts.push(qsTr("Restricted by membership in other rooms or by knocking"));

                        return opts;
                    }
                    currentIndex: roomSettings.accessJoinRules
                    onActivated: {
                        roomSettings.changeAccessRules(index);
                    }
                    Layout.fillWidth: true
                    WheelHandler{} // suppress scrolling changing values
                }

                Label {
                    text: qsTr("Encryption")
                    color: Nheko.colors.text
                }

                ToggleButton {
                    id: encryptionToggle

                    checked: roomSettings.isEncryptionEnabled
                    onCheckedChanged: {
                        if (roomSettings.isEncryptionEnabled) {
                            checked = true;
                            return ;
                        }
                        confirmEncryptionDialog.open();
                    }
                    Layout.alignment: Qt.AlignRight
                }

                Platform.MessageDialog {
                    id: confirmEncryptionDialog

                    title: qsTr("End-to-End Encryption")
                    text: qsTr("Encryption is currently experimental and things might break unexpectedly. <br>
                                Please take note that it can't be disabled afterwards.")
                    modality: Qt.NonModal
                    onAccepted: {
                        if (roomSettings.isEncryptionEnabled)
                            return ;

                        roomSettings.enableEncryption();
                    }
                    onRejected: {
                        encryptionToggle.checked = false;
                    }
                    buttons: Platform.MessageDialog.Ok | Platform.MessageDialog.Cancel
                }

                Label {
                    text: qsTr("Permission")
                    color: Nheko.colors.text
                }

                Button {
                    text: qsTr("Configure")
                    ToolTip.text: qsTr("View and change the permissions in this room")
                    onClicked: timelineRoot.showPLEditor(roomSettings)
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("Aliases")
                    color: Nheko.colors.text
                }

                Button {
                    text: qsTr("Configure")
                    ToolTip.text: qsTr("View and change the addresses/aliases of this room")
                    onClicked: timelineRoot.showAliasEditor(roomSettings)
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("Sticker & Emote Settings")
                    color: Nheko.colors.text
                }

                Button {
                    text: qsTr("Change")
                    ToolTip.text: qsTr("Change what packs are enabled, remove packs or create new ones")
                    onClicked: TimelineManager.openImagePackSettings(roomSettings.roomId)
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("Hidden events")
                    color: Nheko.colors.text
                }

                HiddenEventsDialog {
                    id: hiddenEventsDialog
                    roomid: roomSettings.roomId
                    roomName: roomSettings.roomName
                }

                Button {
                    text: qsTr("Configure")
                    ToolTip.text: qsTr("Select events to hide in this room")
                    onClicked: hiddenEventsDialog.show()
                    Layout.alignment: Qt.AlignRight
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
                    text: qsTr("INFO")
                    font.bold: true
                    color: Nheko.colors.text
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Internal ID")
                    color: Nheko.colors.text
                }

                AbstractButton { // AbstractButton does not allow setting text color
                    Layout.alignment: Qt.AlignRight
                    Layout.fillWidth: true
                    Layout.preferredHeight: idLabel.height
                    Label { // TextEdit does not trigger onClicked
                        id: idLabel
                        text: roomSettings.roomId
                        font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 0.8)
                        color: Nheko.colors.text
                        width: parent.width
                        horizontalAlignment: Text.AlignRight
                        wrapMode: Text.WrapAnywhere
                        ToolTip.text: qsTr("Copied to clipboard")
                        ToolTip.visible: toolTipTimer.running
                    }
                    TextEdit{ // label does not allow selection
                        id: textEdit
                        visible: false
                        text: roomSettings.roomId
                    }
                    onClicked: {
                        textEdit.selectAll()
                        textEdit.copy()
                        toolTipTimer.start()
                    }
                    Timer {
                        id: toolTipTimer
                    }
                }

                Label {
                    text: qsTr("Room Version")
                    color: Nheko.colors.text
                }

                Label {
                    text: roomSettings.roomVersion
                    font.pixelSize: fontMetrics.font.pixelSize
                    Layout.alignment: Qt.AlignRight
                    color: Nheko.colors.text
                }

            }
        }
    }
    Button {
        id: showMoreButton
        anchors.horizontalCenter: flickable.horizontalCenter
        y: Math.min(showMorePlaceholder.y+contentLayout1.y-flickable.contentY,flickable.height-height)
        visible: roomTopic.cut
        text: roomTopic.showMore? qsTr("show less") : qsTr("show more")
        onClicked: {roomTopic.showMore = !roomTopic.showMore
            console.log(flickable.visibleArea)
        }
    }
    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok
        onAccepted: close()
    }
}
