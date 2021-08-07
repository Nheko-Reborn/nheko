// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./ui"
import QtGraphicalEffects 1.0
import QtQuick 2.6
import QtQuick.Controls 2.3
import im.nheko 1.0

Rectangle {
    id: avatar

    property string url
    property string userid
    property string displayName
    property alias textColor: label.color
    property bool crop: true

    signal clicked(var mouse)

    width: 48
    height: 48
    radius: Settings.avatarCircles ? height / 2 : 3
    color: Nheko.colors.alternateBase
    Component.onCompleted: {
        mouseArea.clicked.connect(clicked);
    }

    Label {
        id: label

        anchors.fill: parent
        text: TimelineManager.escapeEmoji(displayName ? String.fromCodePoint(displayName.codePointAt(0)) : "")
        textFormat: Text.RichText
        font.pixelSize: avatar.height / 2
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        visible: img.status != Image.Ready
        color: Nheko.colors.text
    }

    Image {
        id: img

        anchors.fill: parent
        asynchronous: true
        fillMode: avatar.crop ? Image.PreserveAspectCrop : Image.PreserveAspectFit
        mipmap: true
        smooth: true
        sourceSize.width: avatar.width
        sourceSize.height: avatar.height
        layer.enabled: true
        source: avatar.url + ((avatar.crop || !avatar.url) ? "" : "?scale")

        MouseArea {
            id: mouseArea

            anchors.fill: parent

            Ripple {
                rippleTarget: mouseArea
                color: Qt.rgba(Nheko.colors.alternateBase.r, Nheko.colors.alternateBase.g, Nheko.colors.alternateBase.b, 0.5)
            }

        }

        layer.effect: OpacityMask {
            cached: true

            maskSource: Rectangle {
                anchors.fill: parent
                width: avatar.width
                height: avatar.height
                radius: Settings.avatarCircles ? height / 2 : 3
            }

        }

    }

    Rectangle {
        anchors.bottom: avatar.bottom
        anchors.right: avatar.right
        visible: !!userid
        height: avatar.height / 6
        width: height
        radius: Settings.avatarCircles ? height / 2 : height / 4
        color: {
            switch (TimelineManager.userPresence(userid)) {
            case "online":
                return "#00cc66";
            case "unavailable":
                return "#ff9933";
            case "offline":
            default:
                // return "#a82353" don't show anything if offline, since it is confusing, if presence is disabled
                "transparent";
            }
        }
    }

    CursorShape {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

}
