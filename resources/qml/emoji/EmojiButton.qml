import QtQuick 2.10
import QtQuick.Controls 2.1
import im.nheko 1.0
import im.nheko.EmojiModel 1.0

import "../"

ImageButton {
    property var colors: currentActivePalette

    image: ":/icons/icons/ui/smile.png"
    id: emojiButton
    onClicked: emojiPopup.open()

    EmojiPicker {
        id: emojiPopup
        x: Math.round((emojiButton.width - width) / 2)
        y: emojiButton.height
        width: 7 * 52
        height: 6 * 52 
        colors: emojiButton.colors
        model: EmojiProxyModel {
            category: Emoji.Category.People
            sourceModel: EmojiModel {}
        }
    }
}