import "./delegates/"
import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10
import im.nheko 1.0

Rectangle {
    id: replyPopup

    property var room: TimelineManager.timeline

    Layout.fillWidth: true
    visible: room && room.reply
    // Height of child, plus margins, plus border
    implicitHeight: replyPreview.height + 10
    color: colors.window
    z: 3

    Reply {
        id: replyPreview

        anchors.left: parent.left
        anchors.leftMargin: 2 * 22 + 3 * 16
        anchors.right: closeReplyButton.left
        anchors.rightMargin: 2 * 22 + 3 * 16
        anchors.bottom: parent.bottom
        modelData: room ? room.getDump(room.reply, room.id) : {
        }
        userColor: TimelineManager.userColor(modelData.userId, colors.window)
    }

    ImageButton {
        id: closeReplyButton

        anchors.right: parent.right
        anchors.rightMargin: 15
        anchors.top: replyPreview.top
        hoverEnabled: true
        width: 16
        height: 16
        image: ":/icons/icons/ui/remove-symbol.png"
        ToolTip.visible: closeReplyButton.hovered
        ToolTip.text: qsTr("Close")
        onClicked: room.reply = undefined
    }

}
