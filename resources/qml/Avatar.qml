// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./ui"
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import im.nheko 1.0

AbstractButton {
    id: avatar

    property alias color: bg.color
    property bool crop: true
    property string displayName
    property string roomid
    property alias textColor: label.color
    property string url
    property string userid
    property bool showTooltip: true

    height: 48
    width: 48

    background: Rectangle {
        id: bg

        color: palette.alternateBase
        radius: Settings.avatarCircles ? height / 2 : height / 8
    }

    Label {
        id: label

        anchors.fill: parent
        color: palette.text
        enabled: false
        font.pixelSize: avatar.height / 2
        horizontalAlignment: Text.AlignHCenter
        text: TimelineManager.escapeEmoji(avatar.displayName ? String.fromCodePoint(avatar.displayName.codePointAt(0)) : "")
        textFormat: Text.RichText
        verticalAlignment: Text.AlignVCenter
        visible: img.status != Image.Ready && !Settings.useIdenticon
    }
    Image {
        id: identicon

        anchors.fill: parent
        source: Settings.useIdenticon ? ("image://jdenticon/" + (avatar.userid !== "" ? avatar.userid : avatar.roomid) + "?radius=" + (Settings.avatarCircles ? 100 : 25)) : ""
        visible: Settings.useIdenticon && img.status != Image.Ready
    }
    Image {
        id: img

        anchors.fill: parent
        asynchronous: true
        fillMode: avatar.crop ? Image.PreserveAspectCrop : Image.PreserveAspectFit
        source: if (avatar.url.startsWith('image://colorimage')) {
            return avatar.url + "&radius=" + (Settings.avatarCircles ? 100 : 25) + ((avatar.crop) ? "" : "&scale");
        } else if (avatar.url.startsWith('image://')) {
            return avatar.url + "?radius=" + (Settings.avatarCircles ? 100 : 25) + ((avatar.crop) ? "" : "&scale");
        } else if (avatar.url.startsWith(':/')) {
            return "image://colorimage/" + avatar.url + "?" + label.color;
        } else {
            return "";
        }
        sourceSize.height: if (!avatar.url.startsWith('image://MxcImage/') && avatar.url.endsWith('.svg')){
            return avatar.height
        } else {
            return avatar.height * Screen.devicePixelRatio
        }
        sourceSize.width: if (!avatar.url.startsWith('image://MxcImage/') && avatar.url.endsWith('.svg')){
            return avatar.width
        } else {
            return avatar.width * Screen.devicePixelRatio
        }
    }
    Rectangle {
        id: onlineIndicator

        function updatePresence() {
            switch (Presence.userPresence(avatar.userid)) {
            case "online":
                return Nheko.theme.online;
            case "unavailable":
                return Nheko.theme.unavailable;
            case "offline":
            default:
                // return "#a82353" don't show anything if offline, since it is confusing, if presence is disabled
                return "transparent";
            }
        }

        anchors.bottom: avatar.bottom
        anchors.right: avatar.right
        color: updatePresence()
        height: avatar.height / 6
        radius: Settings.avatarCircles ? height / 2 : height / 8
        visible: !!avatar.userid
        width: height

        ToolTip.visible: avatar.showTooltip && ma.containsMouse
        ToolTip.text: Presence.lastActive(avatar.userid)

        MouseArea {
            id: ma
            anchors.fill: parent
            anchors.margins: -10
            hoverEnabled: true
        }

        Connections {
            function onPresenceChanged(id) {
                if (id == avatar.userid)
                    onlineIndicator.color = onlineIndicator.updatePresence();
            }

            target: Presence
        }
    }
    NhekoCursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }
    Ripple {
        color: Qt.rgba(palette.alternateBase.r, palette.alternateBase.g, palette.alternateBase.b, 0.5)
    }
}
