// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import Qt.labs.platform 1.1 as Platform
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import QtQuick.Window 2.15
import im.nheko
import "delegates"

Pane {
    id: topBar

    property string avatarUrl: room ? room.roomAvatarUrl : ""
    property string directChatOtherUserId: room ? room.directChatOtherUserId : ""
    property bool isDirect: room ? room.isDirect : false
    property bool isEncrypted: room ? room.isEncrypted : false
    property string roomId: room ? room.roomId : ""
    property string roomName: room ? room.roomName : qsTr("No room selected")
    property string roomTopic: room ? room.roomTopic : ""
    property bool showBackButton: false
    property int trustlevel: room ? room.trustlevel : Crypto.Unverified

    Layout.fillWidth: true
    implicitHeight: topBarC.height + Nheko.paddingMedium * 2
    padding: 0
    z: 3

    background: Rectangle {
        color: timelineRoot.palette.window
    }
    contentItem: Item {
        GridLayout {
            id: topBarC
            anchors.left: parent.left
            anchors.margins: Nheko.paddingMedium
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            columnSpacing: Nheko.paddingSmall
            rowSpacing: Nheko.paddingSmall

            ImageButton {
                id: backToRoomsButton
                Layout.alignment: Qt.AlignVCenter
                Layout.column: 0
                Layout.preferredHeight: Nheko.avatarSize - Nheko.paddingMedium
                Layout.preferredWidth: Nheko.avatarSize - Nheko.paddingMedium
                Layout.row: 0
                Layout.rowSpan: 2
                ToolTip.text: qsTr("Back to room list")
                ToolTip.visible: hovered
                image: ":/icons/icons/ui/angle-arrow-left.svg"
                visible: showBackButton

                onClicked: Rooms.resetCurrentRoom()
            }
            Avatar {
                Layout.alignment: Qt.AlignVCenter
                Layout.column: 1
                Layout.row: 0
                Layout.rowSpan: 2
                displayName: roomName
                enabled: false
                height: Nheko.avatarSize
                roomid: roomId
                url: avatarUrl.replace("mxc://", "image://MxcImage/")
                userid: isDirect ? directChatOtherUserId : ""
                width: Nheko.avatarSize
            }
            Label {
                Layout.column: 2
                Layout.fillWidth: true
                Layout.row: 0
                color: timelineRoot.palette.text
                elide: Text.ElideRight
                font.pointSize: fontMetrics.font.pointSize * 1.1
                maximumLineCount: 1
                text: roomName
                textFormat: Text.RichText
            }
            MatrixText {
                id: roomTopicC
                Layout.column: 2
                Layout.fillWidth: true
                Layout.maximumHeight: fontMetrics.lineSpacing * 2 // show 2 lines
                Layout.row: 1
                clip: true
                enabled: false
                selectByMouse: false
                text: roomTopic
            }
            AbstractButton {
                Layout.column: 3
                Layout.preferredHeight: Nheko.avatarSize - Nheko.paddingMedium
                Layout.preferredWidth: Nheko.avatarSize - Nheko.paddingMedium
                Layout.row: 0
                Layout.rowSpan: 2
                ToolTip.delay: Nheko.tooltipDelay
                ToolTip.text: {
                    if (!isEncrypted)
                        return qsTr("This room is not encrypted!");
                    switch (trustlevel) {
                    case Crypto.Verified:
                        return qsTr("This room contains only verified devices.");
                    case Crypto.TOFU:
                        return qsTr("This room contains verified devices and devices which have never changed their master key.");
                    default:
                        return qsTr("This room contains unverified devices!");
                    }
                }
                ToolTip.visible: hovered
                background: null

                contentItem: EncryptionIndicator {
                    enabled: false
                    encrypted: isEncrypted
                    sourceSize.height: parent.Layout.preferredHeight * Screen.devicePixelRatio
                    sourceSize.width: parent.Layout.preferredWidth * Screen.devicePixelRatio
                    trust: trustlevel
                    visible: isEncrypted
                }

                onClicked: TimelineManager.openRoomMembers(room)
            }
            ImageButton {
                id: pinButton

                property bool pinsShown: !Settings.hiddenPins.includes(roomId)

                Layout.alignment: Qt.AlignVCenter
                Layout.column: 4
                Layout.preferredHeight: Nheko.avatarSize - Nheko.paddingMedium
                Layout.preferredWidth: Nheko.avatarSize - Nheko.paddingMedium
                Layout.row: 0
                Layout.rowSpan: 2
                ToolTip.text: qsTr("Show or hide pinned messages")
                ToolTip.visible: hovered
                image: pinsShown ? ":/icons/icons/ui/pin.svg" : ":/icons/icons/ui/pin-off.svg"
                visible: !!room && room.pinnedMessages.length > 0

                onClicked: {
                    var ps = Settings.hiddenPins;
                    if (pinsShown) {
                        ps.push(roomId);
                    } else {
                        const index = ps.indexOf(roomId);
                        if (index > -1) {
                            ps.splice(index, 1);
                        }
                    }
                    Settings.hiddenPins = ps;
                }
            }
            ImageButton {
                id: roomOptionsButton
                Layout.alignment: Qt.AlignVCenter
                Layout.column: 5
                Layout.preferredHeight: Nheko.avatarSize - Nheko.paddingMedium
                Layout.preferredWidth: Nheko.avatarSize - Nheko.paddingMedium
                Layout.row: 0
                Layout.rowSpan: 2
                ToolTip.text: qsTr("Room options")
                ToolTip.visible: hovered
                image: ":/icons/icons/ui/options.svg"
                visible: !!room

                onClicked: roomOptionsMenu.open(roomOptionsButton)

                Platform.Menu {
                    id: roomOptionsMenu
                    Platform.MenuItem {
                        text: qsTr("Invite users")
                        visible: room ? room.permissions.canInvite() : false

                        onTriggered: TimelineManager.openInviteUsers(roomId)
                    }
                    Platform.MenuItem {
                        text: qsTr("Members")

                        onTriggered: TimelineManager.openRoomMembers(room)
                    }
                    Platform.MenuItem {
                        text: qsTr("Leave room")

                        onTriggered: TimelineManager.openLeaveRoomDialog(roomId)
                    }
                    Platform.MenuItem {
                        text: qsTr("Settings")

                        onTriggered: TimelineManager.openRoomSettings(roomId)
                    }
                }
            }
            ScrollView {
                id: pinnedMessages
                Layout.column: 2
                Layout.columnSpan: 3
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(contentHeight, Nheko.avatarSize * 4)
                Layout.row: 2
                ScrollBar.horizontal.visible: false
                clip: true
                palette: timelineRoot.palette
                visible: !!room && room.pinnedMessages.length > 0 && !Settings.hiddenPins.includes(roomId)

                ListView {
                    model: room ? room.pinnedMessages : undefined
                    spacing: Nheko.paddingSmall

                    delegate: RowLayout {
                        required property string modelData

                        height: implicitHeight
                        width: ListView.view.width

                        Reply {
                            property var e: room ? room.getDump(modelData, "") : {}

                            Layout.fillWidth: true
                            Layout.preferredHeight: height
                            blurhash: e.blurhash ?? ""
                            body: e.body ?? ""
                            encryptionError: e.encryptionError ?? ""
                            eventId: e.eventId ?? ""
                            filename: e.filename ?? ""
                            filesize: e.filesize ?? ""
                            formattedBody: e.formattedBody ?? ""
                            isOnlyEmoji: e.isOnlyEmoji ?? false
                            originalWidth: e.originalWidth ?? 0
                            proportionalHeight: e.proportionalHeight ?? 1
                            type: e.type ?? MtxEvent.UnknownMessage
                            typeString: e.typeString ?? ""
                            url: e.url ?? ""
                            userColor: TimelineManager.userColor(e.userId, timelineRoot.palette.window)
                            userId: e.userId ?? ""
                            userName: e.userName ?? ""
                        }
                        ImageButton {
                            id: deletePinButton
                            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                            Layout.preferredHeight: 16
                            Layout.preferredWidth: 16
                            ToolTip.text: qsTr("Unpin")
                            ToolTip.visible: hovered
                            hoverEnabled: true
                            image: ":/icons/icons/ui/dismiss.svg"
                            visible: room.permissions.canChange(MtxEvent.PinnedEvents)

                            onClicked: room.unpin(modelData)
                        }
                    }
                }
            }
            ScrollView {
                id: widgets
                Layout.column: 2
                Layout.columnSpan: 3
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(contentHeight, Nheko.avatarSize * 1.5)
                Layout.row: 3
                ScrollBar.horizontal.visible: false
                clip: true
                palette: timelineRoot.palette
                visible: !!room && room.widgetLinks.length > 0 && !Settings.hiddenWidgets.includes(roomId)

                ListView {
                    model: room ? room.widgetLinks : undefined
                    spacing: Nheko.paddingSmall

                    delegate: MatrixText {
                        required property var modelData

                        color: timelineRoot.palette.text
                        text: modelData
                    }
                }
            }
        }
        NhekoCursorShape {
            anchors.bottomMargin: (pinnedMessages.visible ? pinnedMessages.height : 0) + (widgets.visible ? widgets.height : 0)
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
        }
    }

    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds

        onSingleTapped: {
            if (eventPoint.position.y > topBar.height - (pinnedMessages.visible ? pinnedMessages.height : 0) - (widgets.visible ? widgets.height : 0)) {
                eventPoint.accepted = true;
                return;
            }
            if (showBackButton && eventPoint.position.x < Nheko.paddingMedium + backToRoomsButton.width) {
                eventPoint.accepted = true;
                return;
            }
            if (eventPoint.position.x > topBar.width - Nheko.paddingMedium - roomOptionsButton.width) {
                eventPoint.accepted = true;
                return;
            }
            if (room) {
                let p = topBar.mapToItem(roomTopicC, eventPoint.position.x, eventPoint.position.y);
                let link = roomTopicC.linkAt(p.x, p.y);
                if (link) {
                    Nheko.openLink(link);
                } else {
                    TimelineManager.openRoomSettings(room.roomId);
                }
            }
            eventPoint.accepted = true;
        }
    }
    HoverHandler {
        grabPermissions: PointerHandler.TakeOverForbidden | PointerHandler.CanTakeOverFromAnything
    }
}
