import "./delegates"
import "./emoji"
import QtQuick 2.6
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import im.nheko 1.0

Item {
    anchors.left: parent.left
    anchors.right: parent.right
    height: row.height

    MouseArea {
        anchors.fill: parent
        propagateComposedEvents: true
        preventStealing: true
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onClicked: {
            if (mouse.button === Qt.RightButton)
                messageContextMenu.show(model.id, model.type, model.isEncrypted, row);

        }
        onPressAndHold: {
            messageContextMenu.show(model.id, model.type, model.isEncrypted, row, mapToItem(timelineRoot, mouse.x, mouse.y));
        }
    }

    Rectangle {
        color: (Settings.messageHoverHighlight && parent.containsMouse) ? colors.base : "transparent"
        anchors.fill: row
    }

    RowLayout {
        id: row

        anchors.leftMargin: avatarSize + 16
        anchors.left: parent.left
        anchors.right: parent.right

        Column {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 4

            // fancy reply, if this is a reply
            Reply {
                visible: model.replyTo
                modelData: chat.model.getDump(model.replyTo, model.id)
                userColor: TimelineManager.userColor(modelData.userId, colors.base)
            }

            // actual message content
            MessageDelegate {
                id: contentItem

                width: parent.width
                modelData: model
            }

            Reactions {
                id: reactionRow

                reactions: model.reactions
                eventId: model.id
            }

        }

        StatusIndicator {
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.preferredHeight: 16
            width: 16
        }

        EncryptionIndicator {
            visible: model.isRoomEncrypted
            encrypted: model.isEncrypted
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.preferredHeight: 16
            width: 16
        }

        EmojiButton {
            id: reactButton

            visible: Settings.buttonsInTimeline
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.preferredHeight: 16
            width: 16
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr("React")
            emojiPicker: emojiPopup
            event_id: model.id
        }

        ImageButton {
            id: replyButton

            visible: Settings.buttonsInTimeline
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.preferredHeight: 16
            width: 16
            hoverEnabled: true
            image: ":/icons/icons/ui/mail-reply.png"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Reply")
            onClicked: chat.model.replyAction(model.id)
        }

        ImageButton {
            id: optionsButton

            visible: Settings.buttonsInTimeline
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.preferredHeight: 16
            width: 16
            hoverEnabled: true
            image: ":/icons/icons/ui/vertical-ellipsis.png"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Options")
            onClicked: messageContextMenu.show(model.id, model.type, model.isEncrypted, optionsButton)
        }

        Label {
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            text: model.timestamp.toLocaleTimeString("HH:mm")
            width: Math.max(implicitWidth, text.length * fontMetrics.maximumCharacterWidth)
            color: inactiveColors.text
            ToolTip.visible: ma.containsMouse
            ToolTip.text: Qt.formatDateTime(model.timestamp, Qt.DefaultLocaleLongDate)

            MouseArea {
                id: ma

                anchors.fill: parent
                hoverEnabled: true
                propagateComposedEvents: true
            }

        }

    }

}
