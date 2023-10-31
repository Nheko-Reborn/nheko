// SPDX-FileCopyrightText: Nheko Contributors
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

    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: 680
    minimumHeight: 450
    minimumWidth: 340
    modality: Qt.NonModal
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
                id: displayAvatar

                Layout.alignment: Qt.AlignHCenter
                Layout.preferredHeight: 130
                Layout.preferredWidth: 130
                Layout.topMargin: Nheko.paddingMedium
                displayName: roomSettings.roomName
                roomid: roomSettings.roomId
                url: roomSettings.roomAvatarUrl.replace("mxc://", "image://MxcImage/")

                onClicked: TimelineManager.openImageOverlay(null, roomSettings.roomAvatarUrl, "", 0, 0)

                ImageButton {
                    ToolTip.text: qsTr("Change room avatar.")
                    ToolTip.visible: hovered
                    anchors.left: displayAvatar.left
                    anchors.leftMargin: Nheko.paddingMedium
                    anchors.top: displayAvatar.top
                    anchors.topMargin: Nheko.paddingMedium
                    hoverEnabled: true
                    image: ":/icons/icons/ui/edit.svg"
                    visible: roomSettings.canChangeAvatar

                    onClicked: {
                        roomSettings.updateAvatar();
                    }
                }
            }
            Spinner {
                Layout.alignment: Qt.AlignHCenter
                foreground: palette.mid
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
                color: palette.text
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
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Nheko.paddingMedium

                Label {
                    color: palette.text
                    text: qsTr("%n member(s)", "", roomSettings.memberCount)
                }
                ImageButton {
                    ToolTip.text: qsTr("View members of %1").arg(roomSettings.roomName)
                    ToolTip.visible: hovered
                    hoverEnabled: true
                    image: ":/icons/icons/ui/people.svg"

                    onClicked: TimelineManager.openRoomMembers(Rooms.getRoomById(roomSettings.roomId))
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
                color: palette.text
                horizontalAlignment: TextEdit.AlignHCenter
                readOnly: !isTopicEditingAllowed
                text: isTopicEditingAllowed ? roomSettings.plainRoomTopic : (roomSettings.plainRoomTopic === "" ? ("<i>" + qsTr("No topic set") + "</i>") : roomSettings.roomTopic)
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
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    Layout.topMargin: Nheko.paddingLarge
                    color: palette.text
                    font.bold: true
                    text: qsTr("NOTIFICATIONS")
                }
                Label {
                    Layout.fillWidth: true
                    color: palette.text
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
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    Layout.topMargin: Nheko.paddingLarge
                    color: palette.text
                    font.bold: true
                    text: qsTr("ENTRY PERMISSIONS")
                }
                Label {
                    Layout.fillWidth: true
                    color: palette.text
                    text: qsTr("Anyone can join")
                }
                ToggleButton {
                    id: publicRoomButton

                    Layout.alignment: Qt.AlignRight
                    checked: !roomSettings.privateAccess
                    enabled: roomSettings.canChangeJoinRules
                }
                Label {
                    Layout.fillWidth: true
                    color: palette.text
                    text: qsTr("Allow knocking")
                    visible: knockingButton.visible
                }
                ToggleButton {
                    id: knockingButton

                    Layout.alignment: Qt.AlignRight
                    checked: roomSettings.knockingEnabled
                    enabled: roomSettings.canChangeJoinRules && roomSettings.supportsKnocking
                    visible: !publicRoomButton.checked

                    onCheckedChanged: {
                        if (checked && !roomSettings.supportsKnockRestricted)
                            restrictedButton.checked = false;
                    }
                }
                Label {
                    Layout.fillWidth: true
                    color: palette.text
                    text: qsTr("Allow joining via other rooms")
                    visible: restrictedButton.visible
                }
                ToggleButton {
                    id: restrictedButton

                    Layout.alignment: Qt.AlignRight
                    checked: roomSettings.restrictedEnabled
                    enabled: roomSettings.canChangeJoinRules && roomSettings.supportsRestricted
                    visible: !publicRoomButton.checked

                    onCheckedChanged: {
                        if (checked && !roomSettings.supportsKnockRestricted)
                            knockingButton.checked = false;
                    }
                }
                Label {
                    Layout.fillWidth: true
                    color: palette.text
                    text: qsTr("Rooms to join via")
                    visible: allowedRoomsButton.visible
                }
                Button {
                    id: allowedRoomsButton

                    Layout.alignment: Qt.AlignRight
                    ToolTip.text: qsTr("Change the list of rooms users can join this room via. Usually this is the official community of this room.")
                    enabled: roomSettings.canChangeJoinRules && roomSettings.supportsRestricted
                    text: qsTr("Change")
                    visible: restrictedButton.checked && restrictedButton.visible

                    onClicked: timelineRoot.showAllowedRoomsEditor(roomSettings)
                }
                Label {
                    Layout.fillWidth: true
                    color: palette.text
                    text: qsTr("Allow guests to join")
                }
                ToggleButton {
                    id: guestAccessButton

                    Layout.alignment: Qt.AlignRight
                    checked: roomSettings.guestAccess
                    enabled: roomSettings.canChangeJoinRules
                }
                Button {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    enabled: roomSettings.canChangeJoinRules
                    text: qsTr("Apply access rules")
                    visible: publicRoomButton.checked == roomSettings.privateAccess || knockingButton.checked != roomSettings.knockingEnabled || restrictedButton.checked != roomSettings.restrictedEnabled || guestAccessButton.checked != roomSettings.guestAccess || roomSettings.allowedRoomsModified

                    onClicked: roomSettings.changeAccessRules(!publicRoomButton.checked, guestAccessButton.checked, knockingButton.checked, restrictedButton.checked)
                }
                Label {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    Layout.topMargin: Nheko.paddingLarge
                    color: palette.text
                    font.bold: true
                    text: qsTr("MESSAGE VISIBILITY")
                }
                Label {
                    Layout.fillWidth: true
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("This is useful to see previews of the room or view it on public websites.")
                    ToolTip.visible: publicHistoryHover.hovered
                    color: palette.text
                    text: qsTr("Allow viewing history without joining")

                    HoverHandler {
                        id: publicHistoryHover

                    }
                }
                ToggleButton {
                    id: publicHistoryButton

                    Layout.alignment: Qt.AlignRight
                    checked: roomSettings.historyVisibility == RoomSettings.WorldReadable
                    enabled: roomSettings.canChangeHistoryVisibility
                }
                Label {
                    Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                    Layout.fillWidth: true
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("How much of the history is visible to joined members. Changing this won't affect the visibility of already sent messages. It only applies to new messages.")
                    ToolTip.visible: privateHistoryHover.hovered
                    color: palette.text
                    text: qsTr("Members can see messages since")
                    visible: !publicHistoryButton.checked

                    HoverHandler {
                        id: privateHistoryHover

                    }
                }
                ColumnLayout {
                    Layout.alignment: Qt.AlignTop | Qt.AlignRight
                    Layout.fillWidth: true
                    enabled: roomSettings.canChangeHistoryVisibility
                    visible: !publicHistoryButton.checked

                    RadioButton {
                        id: sharedHistory

                        ToolTip.delay: Nheko.tooltipDelay
                        ToolTip.text: qsTr("As long as the user joined, they can see all previous messages.")
                        ToolTip.visible: hovered
                        checked: roomSettings.historyVisibility == RoomSettings.Shared
                        text: qsTr("Everything")
                    }
                    RadioButton {
                        id: invitedHistory

                        ToolTip.delay: Nheko.tooltipDelay
                        ToolTip.text: qsTr("Members can only see messages from when they got invited going forward.")
                        ToolTip.visible: hovered
                        checked: roomSettings.historyVisibility == RoomSettings.Invited
                        text: qsTr("They got invited")
                    }
                    RadioButton {
                        id: joinedHistory

                        ToolTip.delay: Nheko.tooltipDelay
                        ToolTip.text: qsTr("Members can only see messages since after they joined.")
                        ToolTip.visible: hovered
                        checked: roomSettings.historyVisibility == RoomSettings.Joined || roomSettings.historyVisibility == RoomSettings.WorldReadable
                        text: qsTr("They joined")
                    }
                }
                Button {
                    property int selectedVisibility: {
                        if (publicHistoryButton.checked)
                            return RoomSettings.WorldReadable;
                        else if (sharedHistory.checked)
                            return RoomSettings.Shared;
                        else if (invitedHistory.checked)
                            return RoomSettings.Invited;
                        return RoomSettings.Joined;
                    }

                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    enabled: roomSettings.canChangeHistoryVisibility
                    text: qsTr("Apply visibility changes")
                    visible: roomSettings.historyVisibility != selectedVisibility

                    onClicked: roomSettings.changeHistoryVisibility(selectedVisibility)
                }
                Label {
                    color: palette.text
                    text: qsTr("Locally hidden events")
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
                Label {
                    color: palette.text
                    text: qsTr("Automatic event deletion")
                }
                EventExpirationDialog {
                    id: eventExpirationDialog

                    roomName: roomSettings.roomName
                    roomid: roomSettings.roomId
                }
                Button {
                    Layout.alignment: Qt.AlignRight
                    ToolTip.text: qsTr("Select if your events get automatically deleted in this room.")
                    text: qsTr("Configure")

                    onClicked: eventExpirationDialog.show()
                }
                Label {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    Layout.topMargin: Nheko.paddingLarge
                    color: palette.text
                    font.bold: true
                    text: qsTr("GENERAL SETTINGS")
                }
                Label {
                    color: palette.text
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
                        if (checked === true)
                            confirmEncryptionDialog.open();
                    }
                }
                Platform.MessageDialog {
                    id: confirmEncryptionDialog

                    buttons: Platform.MessageDialog.Ok | Platform.MessageDialog.Cancel
                    modality: Qt.NonModal
                    text: qsTr(`Encryption is currently experimental and things might break unexpectedly. <br>
                                Please take note that it can't be disabled afterwards.`)
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
                    color: palette.text
                    text: qsTr("Permission")
                }
                Button {
                    Layout.alignment: Qt.AlignRight
                    ToolTip.text: qsTr("View and change the permissions in this room")
                    text: qsTr("Configure")

                    onClicked: timelineRoot.showPLEditor(roomSettings)
                }
                Label {
                    color: palette.text
                    text: qsTr("Aliases")
                }
                Button {
                    Layout.alignment: Qt.AlignRight
                    ToolTip.text: qsTr("View and change the addresses/aliases of this room")
                    text: qsTr("Configure")

                    onClicked: timelineRoot.showAliasEditor(roomSettings)
                }
                Label {
                    color: palette.text
                    text: qsTr("Sticker & Emote Settings")
                }
                Button {
                    Layout.alignment: Qt.AlignRight
                    ToolTip.text: qsTr("Change what packs are enabled, remove packs, or create new ones")
                    text: qsTr("Change")

                    onClicked: TimelineManager.openImagePackSettings(roomSettings.roomId)
                }
                Label {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    Layout.topMargin: Nheko.paddingLarge
                    color: palette.text
                    font.bold: true
                    text: qsTr("INFO")
                }
                Label {
                    color: palette.text
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
                        color: palette.text
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
                    color: palette.text
                    text: qsTr("Room Version")
                }
                Label {
                    Layout.alignment: Qt.AlignRight
                    color: palette.text
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
