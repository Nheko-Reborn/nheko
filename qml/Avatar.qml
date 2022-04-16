// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import "ui"
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import im.nheko

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
        color: timelineRoot.palette.alternateBase
        radius: Settings.avatarCircles ? height / 2 : height / 8
    }

    Label {
        id: label
        anchors.fill: parent
        color: timelineRoot.palette.text
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
        source: avatar.url ? (avatar.url + "?radius=" + (Settings.avatarCircles ? 100 : 25) + ((avatar.crop) ? "" : "&scale")) : ""
        sourceSize.height: avatar.height * Screen.devicePixelRatio
        sourceSize.width: avatar.width * Screen.devicePixelRatio
    }
    Rectangle {
        id: onlineIndicator
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
    NhekoCursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }
    Ripple {
        color: Qt.rgba(timelineRoot.palette.alternateBase.r, timelineRoot.palette.alternateBase.g, timelineRoot.palette.alternateBase.b, 0.5)
    }
}
