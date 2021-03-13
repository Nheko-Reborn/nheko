import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko 1.0

Popup {
    id: quickSwitcher

    property int textHeight: 32
    property int textMargin: 8

    x: parent.width / 2 - width / 2
    y: parent.height / 4 - height / 2
    width: parent.width / 2
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    parent: Overlay.overlay
    palette: colors

    Overlay.modal: Rectangle {
        color: "#aa1E1E1E"
    }

    MatrixTextField {
        id: roomTextInput

        anchors.fill: parent
        font.pixelSize: Math.ceil(quickSwitcher.textHeight * 0.6)
        padding: textMargin
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
            } else if (event.matches(StandardKey.InsertParagraphSeparator)) {
                completerPopup.finishCompletion()
                event.accepted = true;
            }
        }
    }

    Completer {
        id: completerPopup

        x: roomTextInput.x
        y: roomTextInput.y + roomTextInput.height + textMargin
        width: parent.width
        completerName: "room"
        bottomToTop: false
        fullWidth: true
        avatarHeight: textHeight
        avatarWidth: textHeight
        centerRowContent: false
        rowMargin: 8
        rowSpacing: 6

        closePolicy: Popup.NoAutoClose
    }

    onOpened: {
        completerPopup.open()
        delay(200, function() {
            roomTextInput.forceActiveFocus()
        })
    }

    onClosed: {
        completerPopup.close()
    }

    Connections {
        onCompletionSelected: {
            TimelineManager.setHistoryView(id)
            TimelineManager.highlightRoom(id)
            quickSwitcher.close()
        }
        target: completerPopup
    }

    Timer {
        id: timer
    }

    function delay(delayTime, cb) {
        timer.interval = delayTime;
        timer.repeat = false;
        timer.triggered.connect(cb);
        timer.start();
    }
}
