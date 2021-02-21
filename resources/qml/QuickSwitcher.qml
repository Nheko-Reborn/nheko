import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko 1.0

Popup {
    x: parent.width / 2 - width / 2
    y: parent.height / 4 - height / 2
    width: parent.width / 2
    height: 100
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    parent: Overlay.overlay

    TextInput {
        id: roomTextInput

        anchors.fill: parent
        focus: true

        onTextEdited: {
            completerPopup.completer.setSearchString(text)
        }
    }

    Completer {
        id: completerPopup

        x: roomTextInput.x + 100
        y: roomTextInput.y - 20
        completerName: "room"
        bottomToTop: true
    }

    onOpened: {
        completerPopup.open()
    }
}