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

    minimumWidth: 340
    minimumHeight: 450
    width: 450
    height: 680
    color: palette.window
    modality: Qt.NonModal
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    title: qsTr("Room Settings")

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: roomSettingsDialog.close()
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
                foreground: palette.mid
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
                color: palette.text

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
                    color: palette.text
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
                color: palette.text
                horizontalAlignment: TextEdit.AlignHCenter
                onLinkActivated: Nheko.openLink(link)

                NhekoCursorShape {
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
                    text: qsTr("NOTIFICATIONS")
                    font.bold: true
                    color: palette.text
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    Layout.topMargin: Nheko.paddingLarge
                }

                Label {
                    text: qsTr("Notifications")
                    Layout.fillWidth: true
                    color: palette.text
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
                    text: qsTr("ENTRY PERMISSIONS")
                    font.bold: true
                    color: palette.text
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    Layout.topMargin: Nheko.paddingLarge
                }

                Label {
                    text: qsTr("Anyone can join")
                    Layout.fillWidth: true
                    color: palette.text
                }

                ToggleButton {
                    id: publicRoomButton

                    enabled: roomSettings.canChangeJoinRules
                    checked: !roomSettings.privateAccess
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("Allow knocking")
                    Layout.fillWidth: true
                    color: palette.text
                    visible: knockingButton.visible
                }

                ToggleButton {
                    id: knockingButton

                    visible: !publicRoomButton.checked
                    enabled: roomSettings.canChangeJoinRules && roomSettings.supportsKnocking
                    checked: roomSettings.knockingEnabled
                    onCheckedChanged: {
                        if (checked && !roomSettings.supportsKnockRestricted) restrictedButton.checked = false;
                    }
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("Allow joining via other rooms")
                    Layout.fillWidth: true
                    color: palette.text
                    visible: restrictedButton.visible
                }

                ToggleButton {
                    id: restrictedButton

                    visible: !publicRoomButton.checked
                    enabled: roomSettings.canChangeJoinRules && roomSettings.supportsRestricted
                    checked: roomSettings.restrictedEnabled
                    onCheckedChanged: {
                        if (checked && !roomSettings.supportsKnockRestricted) knockingButton.checked = false;
                    }
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("Rooms to join via")
                    Layout.fillWidth: true
                    color: palette.text
                    visible: allowedRoomsButton.visible
                }

                Button {
                    id: allowedRoomsButton

                    visible: restrictedButton.checked && restrictedButton.visible
                    enabled: roomSettings.canChangeJoinRules && roomSettings.supportsRestricted

                    text: qsTr("Change")
                    ToolTip.text: qsTr("Change the list of rooms users can join this room via. Usually this is the official community of this room.")
                    onClicked: timelineRoot.showAllowedRoomsEditor(roomSettings)
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("Allow guests to join")
                    Layout.fillWidth: true
                    color: palette.text
                }

                ToggleButton {
                    id: guestAccessButton

                    enabled: roomSettings.canChangeJoinRules
                    checked: roomSettings.guestAccess
                    Layout.alignment: Qt.AlignRight
                }

                Button {
                    visible: publicRoomButton.checked == roomSettings.privateAccess || knockingButton.checked != roomSettings.knockingEnabled || restrictedButton.checked != roomSettings.restrictedEnabled || guestAccessButton.checked != roomSettings.guestAccess || roomSettings.allowedRoomsModified
                    enabled: roomSettings.canChangeJoinRules

                    text: qsTr("Apply access rules")
                    onClicked: roomSettings.changeAccessRules(!publicRoomButton.checked, guestAccessButton.checked, knockingButton.checked, restrictedButton.checked)
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("MESSAGE VISIBILITY")
                    font.bold: true
                    color: palette.text
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    Layout.topMargin: Nheko.paddingLarge
                }

                Label {
                    text: qsTr("Allow viewing history without joining")
                    Layout.fillWidth: true
                    color: palette.text
                    ToolTip.text: qsTr("This is useful to see previews of the room or view it on public websites.")
                    ToolTip.visible: publicHistoryHover.hovered
                    ToolTip.delay: Nheko.tooltipDelay

                    HoverHandler {
                        id: publicHistoryHover

                    }
                }

                ToggleButton {
                    id: publicHistoryButton

                    enabled: roomSettings.canChangeHistoryVisibility
                    checked: roomSettings.historyVisibility == RoomSettings.WorldReadable
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    visible: !publicHistoryButton.checked
                    text: qsTr("Members can see messages since")
                    Layout.fillWidth: true
                    color: palette.text
                    Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                    ToolTip.text: qsTr("How much of the history is visible to joined members. Changing this won't affect the visibility of already sent messages. It only applies to new messages.")
                    ToolTip.visible: privateHistoryHover.hovered
                    ToolTip.delay: Nheko.tooltipDelay

                    HoverHandler {
                        id: privateHistoryHover

                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: !publicHistoryButton.checked
                    enabled: roomSettings.canChangeHistoryVisibility
                    Layout.alignment: Qt.AlignTop | Qt.AlignRight

                    RadioButton {
                        id: sharedHistory
                        checked: roomSettings.historyVisibility == RoomSettings.Shared
                        text: qsTr("Everything")
                        ToolTip.text: qsTr("As long as the user joined, they can see all previous messages.")
                        ToolTip.visible: hovered
                        ToolTip.delay: Nheko.tooltipDelay
                    }
                    RadioButton {
                        id: invitedHistory
                        checked: roomSettings.historyVisibility == RoomSettings.Invited
                        text: qsTr("They got invited")
                        ToolTip.text: qsTr("Members can only see messages from when they got invited going forward.")
                        ToolTip.visible: hovered
                        ToolTip.delay: Nheko.tooltipDelay
                    }
                    RadioButton {
                        id: joinedHistory
                        checked: roomSettings.historyVisibility == RoomSettings.Joined || roomSettings.historyVisibility == RoomSettings.WorldReadable
                        text: qsTr("They joined")
                        ToolTip.text: qsTr("Members can only see messages since after they joined.")
                        ToolTip.visible: hovered
                        ToolTip.delay: Nheko.tooltipDelay
                    }
                }

                Button {
                    visible: roomSettings.historyVisibility != selectedVisibility
                    enabled: roomSettings.canChangeHistoryVisibility

                    text: qsTr("Apply visibility changes")
                    property int selectedVisibility: {
                        if (publicHistoryButton.checked)
                            return RoomSettings.WorldReadable;
                        else if (sharedHistory.checked)
                            return RoomSettings.Shared;
                        else if (invitedHistory.checked)
                            return RoomSettings.Invited;
                        return RoomSettings.Joined;
                    }
                    onClicked: roomSettings.changeHistoryVisibility(selectedVisibility)
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Locally hidden events")
                    color: palette.text
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

                Label {
                    text: qsTr("Automatic event deletion")
                    color: palette.text
                }

                EventExpirationDialog {
                    id: eventExpirationDialog
                    roomid: roomSettings.roomId
                    roomName: roomSettings.roomName
                }

                Button {
                    text: qsTr("Configure")
                    ToolTip.text: qsTr("Select if your events get automatically deleted in this room.")
                    onClicked: eventExpirationDialog.show()
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("GENERAL SETTINGS")
                    font.bold: true
                    color: palette.text
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    Layout.topMargin: Nheko.paddingLarge
                }

                Label {
                    text: qsTr("Encryption")
                    color: palette.text
                }

                ToggleButton {
                    id: encryptionToggle

                    checked: roomSettings.isEncryptionEnabled
                    onCheckedChanged: {
                        if (roomSettings.isEncryptionEnabled) {
                            checked = true;
                            return ;
                        }
                        if (checked === true)
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
                    color: palette.text
                }

                Button {
                    text: qsTr("Configure")
                    ToolTip.text: qsTr("View and change the permissions in this room")
                    onClicked: timelineRoot.showPLEditor(roomSettings)
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("Aliases")
                    color: palette.text
                }

                Button {
                    text: qsTr("Configure")
                    ToolTip.text: qsTr("View and change the addresses/aliases of this room")
                    onClicked: timelineRoot.showAliasEditor(roomSettings)
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("Sticker & Emote Settings")
                    color: palette.text
                }

                Button {
                    text: qsTr("Change")
                    ToolTip.text: qsTr("Change what packs are enabled, remove packs, or create new ones")
                    onClicked: TimelineManager.openImagePackSettings(roomSettings.roomId)
                    Layout.alignment: Qt.AlignRight
                }

                Label {
                    text: qsTr("INFO")
                    font.bold: true
                    color: palette.text
                    Layout.columnSpan: 2
                    Layout.topMargin: Nheko.paddingLarge
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Internal ID")
                    color: palette.text
                }

                AbstractButton { // AbstractButton does not allow setting text color
                    Layout.alignment: Qt.AlignRight
                    Layout.fillWidth: true
                    Layout.preferredHeight: idLabel.height
                    Label { // TextEdit does not trigger onClicked
                        id: idLabel
                        text: roomSettings.roomId
                        font.pixelSize: Math.floor(fontMetrics.font.pixelSize * 0.8)
                        color: palette.text
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
                    color: palette.text
                }

                Label {
                    text: roomSettings.roomVersion
                    font.pixelSize: fontMetrics.font.pixelSize
                    Layout.alignment: Qt.AlignRight
                    color: palette.text
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
