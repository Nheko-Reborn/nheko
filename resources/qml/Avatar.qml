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
        text: TimelineManager.escapeEmoji(displayName ? String.fromCodePoint(displayName.codePointAt(0)) : "")
        textFormat: Text.RichText
        verticalAlignment: Text.AlignVCenter
        visible: img.status != Image.Ready && !Settings.useIdenticon
    }
    Image {
        id: identicon

        anchors.fill: parent
        source: Settings.useIdenticon ? ("image://jdenticon/" + (userid !== "" ? userid : roomid) + "?radius=" + (Settings.avatarCircles ? 100 : 25)) : ""
        visible: Settings.useIdenticon && img.status != Image.Ready
    }
    Image {
        id: img

        anchors.fill: parent
        asynchronous: true
        fillMode: avatar.crop ? Image.PreserveAspectCrop : Image.PreserveAspectFit
        mipmap: true
        smooth: true
        source: if (avatar.url.startsWith('image://colorimage')) {
            return avatar.url + "&radius=" + (Settings.avatarCircles ? 100 : 25) + ((avatar.crop) ? "" : "&scale");
        } else if (avatar.url.startsWith('image://')) {
            return avatar.url + "?radius=" + (Settings.avatarCircles ? 100 : 25) + ((avatar.crop) ? "" : "&scale");
        } else if (avatar.url.startsWith(':/')) {
            return "image://colorimage/" + avatar.url + "?" + textColor;
        } else {
            return "";
        }
        sourceSize.height: avatar.height * Screen.devicePixelRatio
        sourceSize.width: avatar.width * Screen.devicePixelRatio
    }
    Rectangle {
        id: onlineIndicator

        function updatePresence() {
            switch (Presence.userPresence(userid)) {
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
        visible: !!userid
        width: height

        Connections {
            function onPresenceChanged(id) {
                if (id == userid)
                    onlineIndicator.color = onlineIndicator.updatePresence();
            }

            target: Presence
        }
    }
    CursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }
    Ripple {
        color: Qt.rgba(palette.alternateBase.r, palette.alternateBase.g, palette.alternateBase.b, 0.5)
    }
}
