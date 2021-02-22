import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko 1.0

Popup {
    id: quickSwitcher
    x: parent.width / 2 - width / 2
    y: parent.height / 4 - height / 2
    width: parent.width / 2
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    parent: Overlay.overlay

    TextInput {
        id: roomTextInput

        focus: true
        anchors.fill: parent
        color: colors.text

        onTextEdited: {
            completerPopup.completer.setSearchString(text)
        }

        Keys.onPressed: {
            if (event.key == Qt.Key_Up && completerPopup.opened) {
                event.accepted = true;
                completerPopup.up();
            } else if (event.key == Qt.Key_Down && completerPopup.opened) {
                event.accepted = true;
                completerPopup.down();
            }
        }
    }

    Completer {
        id: completerPopup

        x: roomTextInput.x
        y: roomTextInput.y + parent.height
        width: parent.width
        completerName: "room"
        bottomToTop: true
        fullWidth: true

        closePolicy: Popup.NoAutoClose
    }

    onOpened: {
        completerPopup.open()
        roomTextInput.forceActiveFocus()
    }

    onClosed: {
        completerPopup.close()
    }

    Connections {
        onCompletionSelected: {
            TimelineManager.setHistoryView(id)
            quickSwitcher.close()
        }
        target: completerPopup
    }
}