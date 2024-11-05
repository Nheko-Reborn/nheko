// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./components"
import "./dialogs"
import "./ui"
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko

Page {
    //leftPadding: Nheko.paddingSmall
    //rightPadding: Nheko.paddingSmall
    property int avatarSize: Math.ceil(fontMetrics.lineSpacing * 2.3)
    property bool collapsed: false

    background: Rectangle {
        color: Nheko.theme.sidebarBackground
    }
    footer: ColumnLayout {
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            color: Nheko.theme.separator
            Layout.preferredHeight: 1
        }
        Pane {
            Layout.alignment: Qt.AlignBottom
            Layout.fillWidth: true
            Layout.minimumHeight: 40
            horizontalPadding: Nheko.paddingMedium
            verticalPadding: 0

            background: Rectangle {
                color: palette.window
            }
            contentItem: RowLayout {
                id: buttonRow

                ImageButton {
                    id: startChatButton

                    Layout.fillWidth: true
                    Layout.margins: Nheko.paddingMedium
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Start a new chat")
                    ToolTip.visible: hovered
                    Layout.preferredHeight: 22
                    Layout.preferredWidth: 22
                    hoverEnabled: true
                    image: ":/icons/icons/ui/add-square-button.svg"

                    onClicked: roomJoinCreateMenu.popup(startChatButton)

                    Menu {
                        id: roomJoinCreateMenu

                        MenuItem {
                            text: qsTr("Join a room")

                            onTriggered: Nheko.openJoinRoomDialog()
                        }
                        MenuItem {
                            text: qsTr("Create a new room")

                            onTriggered: {
                                var createRoom = createRoomComponent.createObject(timelineRoot);
                                createRoom.show();
                                timelineRoot.destroyOnClose(createRoom);
                            }
                        }
                        MenuItem {
                            text: qsTr("Start a direct chat")

                            onTriggered: {
                                var createDirect = createDirectComponent.createObject(timelineRoot);
                                createDirect.show();
                                timelineRoot.destroyOnClose(createDirect);
                            }
                        }
                        MenuItem {
                            text: qsTr("Create a new community")

                            onTriggered: {
                                var createRoom = createRoomComponent.createObject(timelineRoot, {
                                        "space": true
                                    });
                                createRoom.show();
                                timelineRoot.destroyOnClose(createRoom);
                            }
                        }
                    }
                }
                ImageButton {
                    Layout.fillWidth: true
                    Layout.margins: Nheko.paddingMedium
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Room directory")
                    ToolTip.visible: hovered
                    Layout.preferredHeight: 22
                    Layout.preferredWidth: 22
                    hoverEnabled: true
                    image: ":/icons/icons/ui/room-directory.svg"
                    visible: !collapsed

                    onClicked: {
                        var win = roomDirectoryComponent.createObject(timelineRoot);
                        win.show();
                        timelineRoot.destroyOnClose(win);
                    }
                }
                ImageButton {
                    Layout.fillWidth: true
                    Layout.margins: Nheko.paddingMedium
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Search rooms (Ctrl+K)")
                    ToolTip.visible: hovered
                    Layout.preferredHeight: 22
                    Layout.preferredWidth: 22
                    hoverEnabled: true
                    image: ":/icons/icons/ui/search.svg"
                    ripple: false
                    visible: !collapsed

                    onClicked: {
                        var component = Qt.createComponent("qrc:/resources/qml/QuickSwitcher.qml");
                        if (component.status == Component.Ready) {
                            var quickSwitch = component.createObject(timelineRoot);
                            quickSwitch.open();
                            destroyOnClosed(quickSwitch);
                        } else {
                            console.error("Failed to create component: " + component.errorString());
                        }
                    }
                }
                ImageButton {
                    Layout.fillWidth: true
                    Layout.margins: Nheko.paddingMedium
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("User settings")
                    ToolTip.visible: hovered
                    Layout.preferredHeight: 22
                    Layout.preferredWidth: 22
                    hoverEnabled: true
                    image: ":/icons/icons/ui/settings.svg"
                    ripple: false
                    visible: !collapsed

                    onClicked: mainWindow.push(userSettingsPage)
                }
            }
        }
    }
    header: ColumnLayout {
        spacing: 0

        Pane {
            id: userInfoPanel

            function openUserProfile() {
                Nheko.updateUserProfile();
                var component = Qt.createComponent("qrc:/resources/qml/dialogs/UserProfile.qml");
                if (component.status == Component.Ready) {
                    var userProfile = component.createObject(timelineRoot, {
                            "profile": Nheko.currentUser
                        });
                    userProfile.show();
                    timelineRoot.destroyOnClose(userProfile);
                } else {
                    console.error("Failed to create component: " + component.errorString());
                }
            }

            Layout.alignment: Qt.AlignBottom
            Layout.fillWidth: true
            Layout.minimumHeight: 40
            //Layout.preferredHeight: userInfoGrid.implicitHeight + 2 * Nheko.paddingMedium
            padding: Nheko.paddingMedium

            background: Rectangle {
                color: palette.window
            }
            contentItem: RowLayout {
                id: userInfoGrid

                property var profile: Nheko.currentUser

                spacing: Nheko.paddingMedium

                Avatar {
                    id: headerAvatar

                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredHeight: fontMetrics.lineSpacing * 2
                    Layout.preferredWidth: fontMetrics.lineSpacing * 2
                    displayName: userInfoGrid.profile ? userInfoGrid.profile.displayName : ""
                    enabled: false
                    url: (userInfoGrid.profile ? userInfoGrid.profile.avatarUrl : "").replace("mxc://", "image://MxcImage/")
                    userid: userInfoGrid.profile ? userInfoGrid.profile.userid : ""
                }
                ColumnLayout {
                    id: col

                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width - headerAvatar.width - logoutButton.width - Nheko.paddingMedium * 2
                    spacing: 0
                    visible: !collapsed

                    ElidedLabel {
                        Layout.alignment: Qt.AlignBottom
                        elideWidth: col.width
                        font.pointSize: fontMetrics.font.pointSize * 1.1
                        font.weight: Font.DemiBold
                        fullText: userInfoGrid.profile ? userInfoGrid.profile.displayName : ""
                    }
                    ElidedLabel {
                        Layout.alignment: Qt.AlignTop
                        color: palette.buttonText
                        elideWidth: col.width
                        font.pointSize: fontMetrics.font.pointSize * 0.9
                        fullText: userInfoGrid.profile ? userInfoGrid.profile.userid : ""
                    }
                }
                Item {
                }
                ImageButton {
                    id: logoutButton

                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredHeight: fontMetrics.lineSpacing * 2
                    Layout.preferredWidth: fontMetrics.lineSpacing * 2
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Logout")
                    ToolTip.visible: hovered
                    image: ":/icons/icons/ui/power-off.svg"
                    visible: !collapsed

                    onClicked: Nheko.openLogoutDialog()
                }
            }

            InputDialog {
                id: statusDialog

                prompt: qsTr("Enter your status message:")
                title: qsTr("Status Message")

                text: userInfoGrid.profile ? Presence.userStatus(userInfoGrid.profile.userid) : ""

                onAccepted: function (text) {
                    Nheko.setStatusMessage(text);
                }
            }
            Menu {
                id: userInfoMenu

                MenuItem {
                    text: qsTr("Profile settings")

                    onTriggered: userInfoPanel.openUserProfile()
                }
                MenuItem {
                    text: qsTr("Set status message")

                    onTriggered: statusDialog.show()
                }
                MenuSeparator {
                }

                ButtonGroup {
                    id: onlineStateGroup
                }
                MenuItem {
                    text: qsTr("Automatic online status")
                    ButtonGroup.group: onlineStateGroup
                    checkable: true
                    checked: Settings.presence == Settings.AutomaticPresence
                    onTriggered: if (checked) Settings.presence = Settings.AutomaticPresence
                }
                MenuItem {
                    text: qsTr("Online")
                    ButtonGroup.group: onlineStateGroup
                    checkable: true
                    checked: Settings.presence == Settings.Online
                    onTriggered: if (checked) Settings.presence = Settings.Online
                }
                MenuItem {
                    text: qsTr("Unavailable")
                    ButtonGroup.group: onlineStateGroup
                    checkable: true
                    checked: Settings.presence == Settings.Unavailable
                    onTriggered: if (checked) Settings.presence = Settings.Unavailable
                }
                MenuItem {
                    text: qsTr("Offline")
                    ButtonGroup.group: onlineStateGroup
                    checkable: true
                    checked: Settings.presence == Settings.Offline
                    onTriggered: if (checked) Settings.presence = Settings.Offline
                }
            }
            TapHandler {
                id: userTapHandler

                acceptedButtons: Qt.LeftButton
                gesturePolicy: TapHandler.ReleaseWithinBounds
                margin: -Nheko.paddingSmall

                onLongPressed: userInfoMenu.popup(userTapHandler)
                onSingleTapped: userInfoPanel.openUserProfile()
            }
            TapHandler {
                id: userTapHandler2

                acceptedButtons: Qt.RightButton
                gesturePolicy: TapHandler.ReleaseWithinBounds
                margin: -Nheko.paddingSmall

                onSingleTapped: userInfoMenu.popup(userTapHandler2)
            }
        }
        Rectangle {
            Layout.fillWidth: true
            color: Nheko.theme.separator
            Layout.preferredHeight: 2
        }
        Rectangle {
            id: unverifiedStuffBubble

            Layout.fillWidth: true
            color: Qt.lighter(Nheko.theme.orange, verifyButtonHovered.hovered ? 1.2 : 1)
            implicitHeight: explanation.height + Nheko.paddingMedium * 2
            visible: SelfVerificationStatus.status != SelfVerificationStatus.AllVerified

            RowLayout {
                id: unverifiedStuffBubbleContainer

                height: explanation.height + Nheko.paddingMedium * 2
                spacing: 0
                width: parent.width

                Label {
                    id: explanation

                    Layout.fillWidth: true
                    Layout.margins: Nheko.paddingMedium
                    Layout.rightMargin: Nheko.paddingSmall
                    color: palette.buttonText
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

                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    Layout.rightMargin: Nheko.paddingMedium
                    ToolTip.delay: Nheko.tooltipDelay
                    ToolTip.text: qsTr("Close")
                    ToolTip.visible: closeUnverifiedBubble.hovered
                    Layout.preferredHeight: fontMetrics.font.pixelSize
                    hoverEnabled: true
                    image: ":/icons/icons/ui/dismiss.svg"
                    Layout.preferredWidth: fontMetrics.font.pixelSize

                    onClicked: unverifiedStuffBubble.visible = false
                }
            }
            HoverHandler {
                id: verifyButtonHovered

                acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                enabled: !closeUnverifiedBubble.hovered
            }
            TapHandler {
                acceptedButtons: Qt.LeftButton
                enabled: !closeUnverifiedBubble.hovered

                onSingleTapped: {
                    if (SelfVerificationStatus.status == SelfVerificationStatus.UnverifiedDevices)
                        SelfVerificationStatus.verifyUnverifiedDevices();
                    else
                        SelfVerificationStatus.statusChanged();
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            color: Nheko.theme.separator
            Layout.preferredHeight: 1
            visible: unverifiedStuffBubble.visible
        }
    }

    // HACK: https://bugreports.qt.io/browse/QTBUG-83972, qtwayland cannot auto hide menu
    Connections {
        function onHideMenu() {
            userInfoMenu.close();
            roomContextMenu.close();
        }

        target: MainWindow
    }
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
        boundsBehavior: Flickable.StopAtBounds

        Loader {
            source: NHEKO_USE_KIRIGAMI ? "components/KirigamiWheelHandler.qml" : ""
        }

        //reuseItems: true
        ScrollBar.vertical: ScrollBar {
            id: scrollbar

            parent: !collapsed && Settings.scrollbarsInRoomlist ? roomlist : null
        }
        delegate: ItemDelegate {
            id: roomItem

            required property string avatarUrl
            property color backgroundColor: palette.window
            property color bubbleBackground: palette.highlight
            property color bubbleText: palette.highlightedText
            required property string directChatOtherUserId
            required property bool hasLoudNotification
            required property bool hasUnreadMessages
            property color importantText: palette.text
            required property bool isDirect
            required property bool isInvite
            required property bool isSpace
            required property string lastMessage
            required property int notificationCount
            required property string roomId
            required property string roomName
            required property var tags
            required property string time
            property color unimportantText: palette.buttonText

            ToolTip.delay: Nheko.tooltipDelay
            ToolTip.text: roomName
            ToolTip.visible: hovered && collapsed
            height: avatarSize + 2 * Nheko.paddingMedium
            state: "normal"
            width: ListView.view.width - ((scrollbar.interactive && scrollbar.visible && scrollbar.parent) ? scrollbar.width : 0)

            topInset: 0
            bottomInset: 0
            leftInset: 0
            rightInset: 0

            background: Rectangle {
                color: backgroundColor
            }
            states: [
                State {
                    name: "highlight"
                    when: roomItem.hovered && !((Rooms.currentRoom && roomId == Rooms.currentRoom.roomId) || Rooms.currentRoomPreview.roomid == roomId)

                    PropertyChanges {
                        roomItem {
                            backgroundColor: palette.dark
                            bubbleBackground: palette.highlight
                            bubbleText: palette.highlightedText
                            importantText: palette.brightText
                            unimportantText: palette.brightText
                        }
                    }
                },
                State {
                    name: "selected"
                    when: (Rooms.currentRoom && roomId == Rooms.currentRoom.roomId) || Rooms.currentRoomPreview.roomid == roomId

                    PropertyChanges {
                        roomItem {
                            backgroundColor: palette.highlight
                            bubbleBackground: palette.highlightedText
                            bubbleText: palette.highlight
                            importantText: palette.highlightedText
                            unimportantText: palette.highlightedText
                        }
                    }
                }
            ]

            onClicked: {
                console.log("tapped " + roomId);
                if (!Rooms.currentRoom || Rooms.currentRoom.roomId !== roomId)
                    Rooms.setCurrentRoom(roomId);
                else
                    Rooms.resetCurrentRoom();
            }
            onPressAndHold: {
                if (!isInvite)
                    roomContextMenu.show(roomItem, roomId, tags);
            }

            Ripple {
                color: Qt.rgba(palette.dark.r, palette.dark.g, palette.dark.b, 0.5)
            }

            // NOTE(Nico): We want to prevent the touch areas from overlapping. For some reason we need to add 1px of padding for that...
            Item {
                anchors.fill: parent
                anchors.margins: 1

                TapHandler {
                    id: roomItemTh

                    acceptedButtons: Qt.RightButton
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                    gesturePolicy: TapHandler.ReleaseWithinBounds

                    onSingleTapped: {
                        if (!TimelineManager.isInvite)
                            roomContextMenu.show(roomItemTh, roomId, tags);
                    }
                }
            }
            RowLayout {
                anchors.fill: parent
                anchors.margins: Nheko.paddingMedium
                spacing: Nheko.paddingMedium

                Avatar {
                    id: avatar

                    Layout.alignment: Qt.AlignVCenter
                    displayName: roomName
                    enabled: false
                    roomid: roomId
                    url: avatarUrl.replace("mxc://", "image://MxcImage/")
                    userid: isDirect ? directChatOtherUserId : ""
                    Layout.preferredWidth: avatarSize
                    Layout.preferredHeight: avatarSize

                    NotificationBubble {
                        id: collapsedNotificationBubble

                        anchors.bottom: parent.bottom
                        anchors.margins: -Nheko.paddingSmall
                        anchors.right: parent.right
                        bubbleBackgroundColor: roomItem.bubbleBackground
                        bubbleTextColor: roomItem.bubbleText
                        hasLoudNotification: roomItem.hasLoudNotification
                        mayBeVisible: collapsed && (isSpace ? Settings.spaceNotifications : true)
                        notificationCount: roomItem.notificationCount
                    }
                }
                ColumnLayout {
                    id: textContent

                    Layout.alignment: Qt.AlignLeft
                    Layout.minimumWidth: 100
                    Layout.preferredWidth: roomItem.width - avatar.width
                    Layout.preferredHeight: avatar.height
                    spacing: Nheko.paddingSmall
                    visible: !collapsed

                    Item {
                        id: titleRow

                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true
                        Layout.preferredHeight: subtitleText.implicitHeight

                        ElidedLabel {
                            id: titleText

                            anchors.left: parent.left
                            color: roomItem.importantText
                            elideWidth: parent.width - (timestamp.visible ? timestamp.implicitWidth : 0) - (spaceNotificationBubble.visible ? spaceNotificationBubble.implicitWidth : 0)
                            fullText: TimelineManager.htmlEscape(roomName)
                            textFormat: Text.RichText
                        }
                        Label {
                            id: timestamp

                            anchors.baseline: titleText.baseline
                            anchors.right: parent.right
                            color: roomItem.unimportantText
                            font.pixelSize: fontMetrics.font.pixelSize * 0.9
                            text: time
                            visible: !isInvite && !isSpace
                        }
                        NotificationBubble {
                            id: spaceNotificationBubble

                            anchors.right: parent.right
                            bubbleBackgroundColor: roomItem.bubbleBackground
                            bubbleTextColor: roomItem.bubbleText
                            hasLoudNotification: roomItem.hasLoudNotification
                            mayBeVisible: !collapsed && (isSpace ? Settings.spaceNotifications : false)
                            notificationCount: roomItem.notificationCount
                            parent: isSpace ? titleRow : subtextRow
                        }
                    }
                    Item {
                        id: subtextRow

                        Layout.alignment: Qt.AlignBottom
                        Layout.fillWidth: true
                        Layout.preferredHeight: subtitleText.implicitHeight
                        visible: !isSpace

                        ElidedLabel {
                            id: subtitleText

                            anchors.left: parent.left
                            color: roomItem.unimportantText
                            elideWidth: subtextRow.width - (subtextNotificationBubble.visible ? subtextNotificationBubble.implicitWidth : 0)
                            font.pixelSize: fontMetrics.font.pixelSize * 0.9
                            fullText: TimelineManager.htmlEscape(lastMessage)
                            textFormat: Text.RichText
                        }
                        NotificationBubble {
                            id: subtextNotificationBubble

                            anchors.baseline: subtitleText.baseline
                            anchors.right: parent.right
                            bubbleBackgroundColor: roomItem.bubbleBackground
                            bubbleTextColor: roomItem.bubbleText
                            hasLoudNotification: roomItem.hasLoudNotification
                            mayBeVisible: !collapsed
                            notificationCount: roomItem.notificationCount
                        }
                    }
                }
            }
            Rectangle {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                color: palette.highlight
                height: parent.height - Nheko.paddingSmall * 2
                visible: hasUnreadMessages
                width: 3
            }
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

                color: palette.window
                height: 650
                minimumHeight: 150
                minimumWidth: 150
                title: room.plainRoomName
                width: 420

                Component.onCompleted: {
                    MainWindow.addPerRoomWindow(room.roomId || roomPreview.roomid, roomWindowW);
                    Nheko.setTransientParent(roomWindowW, null);
                }
                Component.onDestruction: MainWindow.removePerRoomWindow(room.roomId || roomPreview.roomid, roomWindowW)
                onActiveChanged: {
                    room.lastReadIdOnWindowFocus();
                }

                //flags: Qt.Window | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
                Shortcut {
                    sequence: StandardKey.Cancel

                    onActivated: roomWindowW.close()
                }
                TimelineView {
                    id: timeline

                    anchors.fill: parent
                    privacyScreen: privacyScreen
                    room: roomWindowW.room
                    roomPreview: roomWindowW.roomPreview.roomid ? roomWindowW.roomPreview : null
                }
                PrivacyScreen {
                    id: privacyScreen

                    anchors.fill: parent
                    screenTimeout: Settings.privacyScreenTimeout
                    timelineRoot: timeline
                    visible: Settings.privacyScreen
                    windowTarget: roomWindowW
                }
            }
        }
        Component {
            id: nestedSpaceMenuLevel

            SpaceMenuLevel {
                childMenu: rootSpaceMenu.childMenu
                roomid: roomContextMenu.roomid
            }
        }
        Menu {
            id: roomContextMenu

            property string roomid
            property var tags

            function show(parent, roomid_, tags_) {
                roomid = roomid_;
                tags = tags_;
                popup(parent);
            }

            InputDialog {
                id: newTag

                prompt: qsTr("Enter the tag you want to use:")
                title: qsTr("New tag")

                onAccepted: function (text) {
                    Rooms.toggleTag(roomContextMenu.roomid, "u." + text, true);
                }
            }
            MenuItem {
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
            MenuItem {
                text: qsTr("Mark as read")

                onTriggered: Rooms.getRoomById(roomContextMenu.roomid).markRoomAsRead()
            }
            MenuItem {
                text: qsTr("Room settings")

                onTriggered: TimelineManager.openRoomSettings(roomContextMenu.roomid)
            }
            MenuItem {
                text: qsTr("Leave room")

                onTriggered: TimelineManager.openLeaveRoomDialog(roomContextMenu.roomid)
            }
            MenuItem {
                text: qsTr("Copy room link")

                onTriggered: Rooms.copyLink(roomContextMenu.roomid)
            }
            Menu {
                id: tagsMenu

                title: qsTr("Tag room as:")

                Instantiator {
                    model: Communities.tagsWithDefault

                    delegate: MenuItem {
                        property string t: modelData

                        checkable: true
                        checked: roomContextMenu.tags !== undefined && roomContextMenu.tags.includes(t)
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

                        onTriggered: Rooms.toggleTag(roomContextMenu.roomid, t, checked)
                    }

                    onObjectAdded: (index, object) => tagsMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => tagsMenu.removeItem(object)
                }
                MenuItem {
                    text: qsTr("Create new tag...")

                    onTriggered: newTag.show()
                }
            }
            SpaceMenuLevel {
                id: rootSpaceMenu

                childMenu: nestedSpaceMenuLevel
                position: -1
                roomid: roomContextMenu.roomid
                title: qsTr("Add or remove from community...")
            }
        }
    }
}
