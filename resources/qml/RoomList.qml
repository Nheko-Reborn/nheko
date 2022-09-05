// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./components"
import "./dialogs"
import "./ui"
import Qt.labs.platform 1.1 as Platform
import QtQml 2.12
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import im.nheko 1.0

Page {
    //leftPadding: Nheko.paddingSmall
    //rightPadding: Nheko.paddingSmall
    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 2.3)
    property bool collapsed: false

    Component {
        id: roomDirectoryComponent

        RoomDirectory {
        }

    }

    Component {
        id: createRoomComponent

        CreateRoom {
        }
    }

    Component {
        id: createDirectComponent

        CreateDirect {
        }
    }

    ListView {
        id: roomlist

        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        model: Rooms
        reuseItems: true

        ScrollHelper {
            flickable: parent
            anchors.fill: parent
        }

        Connections {
            function onCurrentRoomChanged() {
                if (Rooms.currentRoom)
                    roomlist.positionViewAtIndex(Rooms.roomidToIndex(Rooms.currentRoom.roomId), ListView.Contain);

            }

            target: Rooms
        }

        Component {
            id: roomWindowComponent

            ApplicationWindow {
                id: roomWindowW

                property var room: null
                property var roomPreview: null

                Component.onCompleted: {
                    MainWindow.addPerRoomWindow(room.roomId || roomPreview.roomid, roomWindowW);
                    Nheko.setTransientParent(roomWindowW, null);
                }
                Component.onDestruction: MainWindow.removePerRoomWindow(room.roomId || roomPreview.roomid, roomWindowW)

                height: 650
                width: 420
                minimumWidth: 150
                minimumHeight: 150
                palette: Nheko.colors
                color: Nheko.colors.window
                title: room.plainRoomName
                //flags: Qt.Window | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

                Shortcut {
                    sequence: StandardKey.Cancel
                    onActivated: roomWindowW.close()
                }

                TimelineView {
                    id: timelineView
                    anchors.fill: parent
                    room: roomWindowW.room
                    roomPreview: roomWindowW.roomPreview.roomid ? roomWindowW.roomPreview : null
                }

                PrivacyScreen {
                    anchors.fill: parent
                    visible: Settings.privacyScreen
                    screenTimeout: Settings.privacyScreenTimeout
                    timelineRoot: timelineView
                    windowTarget: roomWindowW
                }
            }

        }


        Component {
            id: nestedSpaceMenuLevel

            SpaceMenuLevel {
                roomid: roomContextMenu.roomid
                childMenu: rootSpaceMenu.childMenu
            }
        }


        Platform.Menu {
            id: roomContextMenu

            property string roomid
            property var tags

            function show(roomid_, tags_) {
                roomid = roomid_;
                tags = tags_;
                open();
            }

            InputDialog {
                id: newTag

                title: qsTr("New tag")
                prompt: qsTr("Enter the tag you want to use:")
                onAccepted: function(text) {
                    Rooms.toggleTag(roomContextMenu.roomid, "u." + text, true);
                }
            }

            Platform.MenuItem {
                text: qsTr("Open separately")
                onTriggered: {
                    var roomWindow = roomWindowComponent.createObject(null, {
                    "room": Rooms.getRoomById(roomContextMenu.roomid),
                    "roomPreview": Rooms.getRoomPreviewById(roomContextMenu.roomid)
                    });
                    roomWindow.showNormal();
                    destroyOnClose(roomWindow);
                }
            }

            Platform.MenuItem {
                text: qsTr("Leave room")
                onTriggered: TimelineManager.openLeaveRoomDialog(roomContextMenu.roomid)
            }

            Platform.MenuItem {
                text: qsTr("Copy room link")
                onTriggered: Rooms.copyLink(roomContextMenu.roomid)
            }

            Platform.Menu {
                id: tagsMenu
                title: qsTr("Tag room as:")

                Instantiator {
                    model: Communities.tagsWithDefault
                    onObjectAdded: tagsMenu.insertItem(index, object)
                    onObjectRemoved: tagsMenu.removeItem(object)

                    delegate: Platform.MenuItem {
                        property string t: modelData

                        text: {
                            switch (t) {
                                case "m.favourite":
                                return qsTr("Favourite");
                                case "m.lowpriority":
                                return qsTr("Low priority");
                                case "m.server_notice":
                                return qsTr("Server notice");
                                default:
                                return t.substring(2);
                            }
                        }
                        checkable: true
                        checked: roomContextMenu.tags !== undefined && roomContextMenu.tags.includes(t)
                        onTriggered: Rooms.toggleTag(roomContextMenu.roomid, t, checked)
                    }

                }

                Platform.MenuItem {
                    text: qsTr("Create new tag...")
                    onTriggered: newTag.show()
                }
            }

            SpaceMenuLevel {
                id: rootSpaceMenu

                roomid: roomContextMenu.roomid
                position: -1
                title: qsTr("Add or remove from space")
                childMenu: nestedSpaceMenuLevel
            }
        }

        delegate: ItemDelegate {
            id: roomItem

            property color backgroundColor: Nheko.colors.window
            property color importantText: Nheko.colors.text
            property color unimportantText: Nheko.colors.buttonText
            property color bubbleBackground: Nheko.colors.highlight
            property color bubbleText: Nheko.colors.highlightedText
            required property string roomName
            required property string roomId
            required property string avatarUrl
            required property string time
            required property string lastMessage
            required property var tags
            required property bool isInvite
            required property bool isSpace
            required property int notificationCount
            required property bool hasLoudNotification
            required property bool hasUnreadMessages
            required property bool isDirect
            required property string directChatOtherUserId

            Ripple {
                color: Qt.rgba(Nheko.colors.dark.r, Nheko.colors.dark.g, Nheko.colors.dark.b, 0.5)
            }

            height: avatarSize + 2 * Nheko.paddingMedium
            width: ListView.view.width
            state: "normal"
            ToolTip.visible: hovered && collapsed
            ToolTip.delay: Nheko.tooltipDelay
            ToolTip.text: roomName
            onClicked: {
                console.log("tapped " + roomId);

                if (isSpace && Communities.currentTagId != "space:"+roomId)
                    Communities.currentTagId = "space:"+roomId;

                if (!Rooms.currentRoom || Rooms.currentRoom.roomId !== roomId)
                    Rooms.setCurrentRoom(roomId);
                else
                    Rooms.resetCurrentRoom();
            }
            onPressAndHold: {
                if (!isInvite)
                    roomContextMenu.show(roomId, tags);

            }
            states: [
                State {
                    name: "highlight"
                    when: roomItem.hovered && !((Rooms.currentRoom && roomId == Rooms.currentRoom.roomId) || Rooms.currentRoomPreview.roomid == roomId)

                    PropertyChanges {
                        target: roomItem
                        backgroundColor: Nheko.colors.dark
                        importantText: Nheko.colors.brightText
                        unimportantText: Nheko.colors.brightText
                        bubbleBackground: Nheko.colors.highlight
                        bubbleText: Nheko.colors.highlightedText
                    }

                },
                State {
                    name: "selected"
                    when: (Rooms.currentRoom && roomId == Rooms.currentRoom.roomId) || Rooms.currentRoomPreview.roomid == roomId

                    PropertyChanges {
                        target: roomItem
                        backgroundColor: Nheko.colors.highlight
                        importantText: Nheko.colors.highlightedText
                        unimportantText: Nheko.colors.highlightedText
                        bubbleBackground: Nheko.colors.highlightedText
                        bubbleText: Nheko.colors.highlight
                    }

                }
            ]

            // NOTE(Nico): We want to prevent the touch areas from overlapping. For some reason we need to add 1px of padding for that...
            Item {
                anchors.fill: parent
                anchors.margins: 1

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onSingleTapped: {
                        if (!TimelineManager.isInvite)
                            roomContextMenu.show(roomId, tags);

                    }
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                }

            }

            RowLayout {
                spacing: Nheko.paddingMedium
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium

                Avatar {
                    id: avatar

                    enabled: false
                    Layout.alignment: Qt.AlignVCenter
                    height: avatarSize
                    width: avatarSize
                    url: avatarUrl.replace("mxc://", "image://MxcImage/")
                    displayName: roomName
                    userid: isDirect ? directChatOtherUserId : ""
                    roomid: roomId

                    NotificationBubble {
                        id: collapsedNotificationBubble

                        notificationCount: roomItem.notificationCount
                        hasLoudNotification: roomItem.hasLoudNotification
                        bubbleBackgroundColor: roomItem.bubbleBackground
                        bubbleTextColor: roomItem.bubbleText
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: -Nheko.paddingSmall
                        mayBeVisible: collapsed && (isSpace ? Settings.spaceNotifications : true)
                    }

                }

                ColumnLayout {
                    id: textContent

                    visible: !collapsed
                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                    Layout.minimumWidth: 100
                    width: parent.width - avatar.width
                    Layout.preferredWidth: parent.width - avatar.width
                    height: avatar.height
                    spacing: Nheko.paddingSmall

                    NotificationBubble {
                        id: notificationBubble

                        parent: isSpace ? titleRow : subtextRow
                        notificationCount: roomItem.notificationCount
                        hasLoudNotification: roomItem.hasLoudNotification
                        bubbleBackgroundColor: roomItem.bubbleBackground
                        bubbleTextColor: roomItem.bubbleText
                        Layout.alignment: Qt.AlignRight
                        Layout.leftMargin: Nheko.paddingSmall
                        Layout.preferredWidth: implicitWidth
                        Layout.preferredHeight: implicitHeight
                        mayBeVisible: !collapsed && (isSpace ? Settings.spaceNotifications : true)
                    }

                    RowLayout {
                        id: titleRow

                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true
                        spacing: Nheko.paddingSmall

                        ElidedLabel {
                            id: rN
                            Layout.alignment: Qt.AlignBaseline
                            color: roomItem.importantText
                            elideWidth: width
                            fullText: roomName
                            textFormat: Text.RichText
                            Layout.fillWidth: true
                        }

                        Label {
                            id: timestamp

                            visible: !isInvite && !isSpace
                            width: visible ? 0 : undefined
                            Layout.alignment: Qt.AlignRight | Qt.AlignBaseline
                            font.pixelSize: fontMetrics.font.pixelSize * 0.9
                            color: roomItem.unimportantText
                            text: time
                        }

                    }

                    RowLayout {
                        id: subtextRow

                        Layout.fillWidth: true
                        spacing: 0
                        visible: !isSpace
                        height: visible ? 0 : undefined
                        Layout.alignment: Qt.AlignBottom

                        ElidedLabel {
                            color: roomItem.unimportantText
                            font.pixelSize: fontMetrics.font.pixelSize * 0.9
                            elideWidth: width
                            fullText: lastMessage
                            textFormat: Text.RichText
                            Layout.fillWidth: true
                        }

                    }

                }

            }

            Rectangle {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                height: parent.height - Nheko.paddingSmall * 2
                width: 3
                color: Nheko.colors.highlight
                visible: hasUnreadMessages
            }

            background: Rectangle {
                color: backgroundColor
            }

        }

    }

    background: Rectangle {
        color: Nheko.theme.sidebarBackground
    }

    header: ColumnLayout {
        spacing: 0

        Pane {
            id: userInfoPanel

            function openUserProfile() {
                Nheko.updateUserProfile();
                var userProfile = userProfileComponent.createObject(timelineRoot, {
                    "profile": Nheko.currentUser
                });
                userProfile.show();
                timelineRoot.destroyOnClose(userProfile);
            }


            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            //Layout.preferredHeight: userInfoGrid.implicitHeight + 2 * Nheko.paddingMedium
            padding: Nheko.paddingMedium
            Layout.minimumHeight: 40

            background: Rectangle {color: Nheko.colors.window}

            InputDialog {
                id: statusDialog

                title: qsTr("Status Message")
                prompt: qsTr("Enter your status message:")
                onAccepted: function(text) {
                    Nheko.setStatusMessage(text);
                }
            }

            Platform.Menu {
                id: userInfoMenu

                Platform.MenuItem {
                    text: qsTr("Profile settings")
                    onTriggered: userInfoPanel.openUserProfile()
                }

                Platform.MenuItem {
                    text: qsTr("Set status message")
                    onTriggered: statusDialog.show()
                }

            }

            TapHandler {
                margin: -Nheko.paddingSmall
                acceptedButtons: Qt.LeftButton
                onSingleTapped: userInfoPanel.openUserProfile()
                onLongPressed: userInfoMenu.open()
                gesturePolicy: TapHandler.ReleaseWithinBounds
            }

            TapHandler {
                margin: -Nheko.paddingSmall
                acceptedButtons: Qt.RightButton
                onSingleTapped: userInfoMenu.open()
                gesturePolicy: TapHandler.ReleaseWithinBounds
            }

            contentItem: RowLayout {
                id: userInfoGrid

                property var profile: Nheko.currentUser

                spacing: Nheko.paddingMedium

                Avatar {
                    id: avatar

                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: fontMetrics.lineSpacing * 2
                    Layout.preferredHeight: fontMetrics.lineSpacing * 2
                    url: (userInfoGrid.profile ? userInfoGrid.profile.avatarUrl : "").replace("mxc://", "image://MxcImage/")
                    displayName: userInfoGrid.profile ? userInfoGrid.profile.displayName : ""
                    userid: userInfoGrid.profile ? userInfoGrid.profile.userid : ""
                    enabled: false
                }

                ColumnLayout {
                    id: col

                    visible: !collapsed
                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                    width: parent.width - avatar.width - logoutButton.width - Nheko.paddingMedium * 2
                    Layout.preferredWidth: parent.width - avatar.width - logoutButton.width - Nheko.paddingMedium * 2
                    spacing: 0

                    ElidedLabel {
                        Layout.alignment: Qt.AlignBottom
                        font.pointSize: fontMetrics.font.pointSize * 1.1
                        font.weight: Font.DemiBold
                        fullText: userInfoGrid.profile ? userInfoGrid.profile.displayName : ""
                        elideWidth: col.width
                    }

                    ElidedLabel {
                        Layout.alignment: Qt.AlignTop
                        color: Nheko.colors.buttonText
                        font.pointSize: fontMetrics.font.pointSize * 0.9
                        elideWidth: col.width
                        fullText: userInfoGrid.profile ? userInfoGrid.profile.userid : ""
                    }

                }

                Item {
                }

                ImageButton {
                    id: logoutButton

                    visible: !collapsed
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: fontMetrics.lineSpacing * 2
                    Layout.preferredHeight: fontMetrics.lineSpacing * 2
                    image: ":/icons/icons/ui/power-off.svg"
                    ToolTip.visible: hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Logout")
                    onClicked: Nheko.openLogoutDialog()
                }

            }

        }

        Rectangle {
            color: Nheko.theme.separator
            height: 2
            Layout.fillWidth: true
        }

        Rectangle {
            id: unverifiedStuffBubble

            color: Qt.lighter(Nheko.theme.orange, verifyButtonHovered.hovered ? 1.2 : 1)
            Layout.fillWidth: true
            implicitHeight: explanation.height + Nheko.paddingMedium * 2
            visible: SelfVerificationStatus.status != SelfVerificationStatus.AllVerified

            RowLayout {
                id: unverifiedStuffBubbleContainer

                width: parent.width
                height: explanation.height + Nheko.paddingMedium * 2
                spacing: 0

                Label {
                    id: explanation

                    Layout.margins: Nheko.paddingMedium
                    Layout.rightMargin: Nheko.paddingSmall
                    color: Nheko.colors.buttonText
                    Layout.fillWidth: true
                    text: {
                        switch (SelfVerificationStatus.status) {
                        case SelfVerificationStatus.NoMasterKey:
                            //: Cross-signing setup has not run yet.
                            return qsTr("Encryption not set up");
                        case SelfVerificationStatus.UnverifiedMasterKey:
                            //: The user just signed in with this device and hasn't verified their master key.
                            return qsTr("Unverified login");
                        case SelfVerificationStatus.UnverifiedDevices:
                            //: There are unverified devices signed in to this account.
                            return qsTr("Please verify your other devices");
                        default:
                            return "";
                        }
                    }
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }

                ImageButton {
                    id: closeUnverifiedBubble

                    Layout.rightMargin: Nheko.paddingMedium
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    hoverEnabled: true
                    width: fontMetrics.font.pixelSize
                    height: fontMetrics.font.pixelSize
                    image: ":/icons/icons/ui/dismiss.svg"
                    ToolTip.visible: closeUnverifiedBubble.hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Close")
                    onClicked: unverifiedStuffBubble.visible = false
                }

            }

            HoverHandler {
                id: verifyButtonHovered

                enabled: !closeUnverifiedBubble.hovered
                acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
            }

            TapHandler {
                enabled: !closeUnverifiedBubble.hovered
                acceptedButtons: Qt.LeftButton
                onSingleTapped: {
                    if (SelfVerificationStatus.status == SelfVerificationStatus.UnverifiedDevices)
                        SelfVerificationStatus.verifyUnverifiedDevices();
                    else
                        SelfVerificationStatus.statusChanged();
                }
            }

        }

        Rectangle {
            color: Nheko.theme.separator
            height: 1
            Layout.fillWidth: true
            visible: unverifiedStuffBubble.visible
        }

    }

    footer: ColumnLayout {
        spacing: 0

        Rectangle {
            color: Nheko.theme.separator
            height: 1
            Layout.fillWidth: true
        }

        Pane {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            Layout.minimumHeight: 40

            horizontalPadding: Nheko.paddingMedium
            verticalPadding: 0

            background: Rectangle {color: Nheko.colors.window}
            contentItem: RowLayout {
                id: buttonRow

                ImageButton {
                    Layout.fillWidth: true
                    hoverEnabled: true
                    width: 22
                    height: 22
                    image: ":/icons/icons/ui/add-square-button.svg"
                    ToolTip.visible: hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Start a new chat")
                    Layout.margins: Nheko.paddingMedium
                    onClicked: roomJoinCreateMenu.open(parent)

                    Platform.Menu {
                        id: roomJoinCreateMenu

                        Platform.MenuItem {
                            text: qsTr("Join a room")
                            onTriggered: Nheko.openJoinRoomDialog()
                        }

                        Platform.MenuItem {
                            text: qsTr("Create a new room")
                            onTriggered: {
                                var createRoom = createRoomComponent.createObject(timelineRoot);
                                createRoom.show();
                                timelineRoot.destroyOnClose(createRoom);
                            }
                        }

                        Platform.MenuItem {
                            text: qsTr("Start a direct chat")
                            onTriggered: {
                                var createDirect = createDirectComponent.createObject(timelineRoot);
                                createDirect.show();
                                timelineRoot.destroyOnClose(createDirect);
                            }
                        }

                        Platform.MenuItem {
                            text: qsTr("Create a new community")
                            onTriggered: {
                                var createRoom = createRoomComponent.createObject(timelineRoot, { "space": true });
                                createRoom.show();
                                timelineRoot.destroyOnClose(createRoom);
                            }
                        }

                    }

                }

                ImageButton {
                    visible: !collapsed
                    Layout.fillWidth: true
                    hoverEnabled: true
                    width: 22
                    height: 22
                    image: ":/icons/icons/ui/room-directory.svg"
                    ToolTip.visible: hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Room directory")
                    Layout.margins: Nheko.paddingMedium
                    onClicked: {
                        var win = roomDirectoryComponent.createObject(timelineRoot);
                        win.show();
                        timelineRoot.destroyOnClose(win);
                    }
                }

                ImageButton {
                    visible: !collapsed
                    Layout.fillWidth: true
                    hoverEnabled: true
                    ripple: false
                    width: 22
                    height: 22
                    image: ":/icons/icons/ui/search.svg"
                    ToolTip.visible: hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Search rooms (Ctrl+K)")
                    Layout.margins: Nheko.paddingMedium
                    onClicked: {
                        var quickSwitch = quickSwitcherComponent.createObject(timelineRoot);
                        quickSwitch.open();
                        timelineRoot.destroyOnClose(quickSwitch);
                    }
                }

                ImageButton {
                    visible: !collapsed
                    Layout.fillWidth: true
                    hoverEnabled: true
                    ripple: false
                    width: 22
                    height: 22
                    image: ":/icons/icons/ui/settings.svg"
                    ToolTip.visible: hovered
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("User settings")
                    Layout.margins: Nheko.paddingMedium
                    onClicked: mainWindow.push(userSettingsPage);
                }

            }

        }

    }

}
