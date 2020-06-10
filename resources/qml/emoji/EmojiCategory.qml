import QtQuick 2.9
import QtQuick.Controls 2.9
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.9

import im.nheko 1.0
import im.nheko.EmojiModel 1.0

GridView {
    id: root
    property var category
    property var emojiPopup
    property EmojiProxyModel model: EmojiProxyModel {
        sourceModel: EmojiModel {
            viewCategory: category
        }
    }

    interactive: false

    cellWidth: 52
    cellHeight: 52
    height: 52 * ( model.count / 7 + 1 )

    clip: true

    // Individual emoji
    delegate: AbstractButton {
        width: 48
        height: 48
        contentItem: Text {
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: settings.emoji_font_family
            
            font.pixelSize: 36
            text: model.unicode
        }

        background: Rectangle {
            anchors.fill: parent
            color: hovered ? colors.highlight : 'transparent'
            radius: 5
        }

        hoverEnabled: true
        ToolTip.text: model.shortName
        ToolTip.visible: hovered

        // give the emoji a little oomf
        DropShadow {
            width: parent.width;
            height: parent.height;
            horizontalOffset: 3
            verticalOffset: 3
            radius: 8.0
            samples: 17
            color: "#80000000"
            source: parent.contentItem
        }
        // TODO: maybe add favorites at some point?
        onClicked: {
            console.debug("Picked " + model.unicode + "in response to " + emojiPopup.event_id + " in room " + emojiPopup.room_id)
            emojiPopup.picked(emojiPopup.room_id, emojiPopup.event_id, model.unicode)
        }
    }
}