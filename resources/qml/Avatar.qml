import QtGraphicalEffects 1.0
import QtQuick 2.6
import QtQuick.Controls 2.3
import im.nheko 1.0

Rectangle {
    id: avatar

    property alias url: img.source
    property string userid
    property string displayName
 
    property string usrUrl: TimelineManager.timeline.avatarUrl(userid)

    width: 48
    height: 48
    radius: Settings.avatarCircles ? height / 2 : 3
    color: colors.alternateBase

    Label {
        anchors.fill: parent
        text: TimelineManager.escapeEmoji(displayName ? String.fromCodePoint(displayName.codePointAt(0)) : "")
        textFormat: Text.RichText
        font.pixelSize: avatar.height / 2
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        visible: img.status != Image.Ready
        color: colors.text
    }

    Image {
        id: img

        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        mipmap: true
        smooth: true
        sourceSize.width: avatar.width
        sourceSize.height: avatar.height
        layer.enabled: true

        layer.effect: OpacityMask {

            maskSource: Rectangle {
                anchors.fill: parent
                width: avatar.width
                height: avatar.height
                radius: Settings.avatarCircles ? height / 2 : 3
            }

        }

        MouseArea {
            id: mouseArea

            anchors.fill: parent
            onClicked: {
                TimelineManager.openImageOverlay(usrUrl, TimelineManager.timeline.data.id)
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

}
