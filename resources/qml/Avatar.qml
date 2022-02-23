// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./ui"
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import im.nheko 1.0

AbstractButton {
    id: avatar

    property string url
    property string userid
    property string roomid
    property string displayName
    property alias textColor: label.color
    property bool crop: true
    property alias color: bg.color

    width: 48
    height: 48
    background: Rectangle {
        id: bg
        radius: Settings.avatarCircles ? height / 2 : height / 8
        color: Nheko.colors.alternateBase
    }

    Label {
        id: label

        enabled: false

        anchors.fill: parent
        text: TimelineManager.escapeEmoji(displayName ? String.fromCodePoint(displayName.codePointAt(0)) : "")
        textFormat: Text.RichText
        font.pixelSize: avatar.height / 2
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        visible: img.status != Image.Ready && !Settings.useIdenticon
        color: Nheko.colors.text
    }

    Image {
        id: identicon

        anchors.fill: parent
        visible: Settings.useIdenticon && img.status != Image.Ready
        source: Settings.useIdenticon ? ("image://jdenticon/" + (userid !== "" ? userid : roomid) + "?radius=" + (Settings.avatarCircles ? 100 : 25)) : ""
    }

    Image {
        id: img

        anchors.fill: parent
        asynchronous: true
        fillMode: avatar.crop ? Image.PreserveAspectCrop : Image.PreserveAspectFit
        mipmap: true
        smooth: true
        sourceSize.width: avatar.width * Screen.devicePixelRatio
        sourceSize.height: avatar.height * Screen.devicePixelRatio
        source: avatar.url ? (avatar.url + "?radius=" + (Settings.avatarCircles ? 100 : 25) + ((avatar.crop) ? "" : "&scale")) : ""

    }

    Rectangle {
        id: onlineIndicator

        anchors.bottom: avatar.bottom
        anchors.right: avatar.right
        visible: !!userid
        height: avatar.height / 6
        width: height
        radius: Settings.avatarCircles ? height / 2 : height / 8
        color: updatePresence()

        function updatePresence() {
            switch (Presence.userPresence(userid)) {
            case "online":
                return "#00cc66";
            case "unavailable":
                return "#ff9933";
            case "offline":
            default:
                // return "#a82353" don't show anything if offline, since it is confusing, if presence is disabled
                return "transparent";
            }
        }

        Connections {
            target: Presence

            function onPresenceChanged(id) {
                if (id == userid) onlineIndicator.color = onlineIndicator.updatePresence();
            }
        }
    }

    CursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

    Ripple {
        color: Qt.rgba(Nheko.colors.alternateBase.r, Nheko.colors.alternateBase.g, Nheko.colors.alternateBase.b, 0.5)
    }

}
