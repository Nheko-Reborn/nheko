import QtQuick 2.9
import QtQuick.Controls 2.3
import im.nheko 1.0

Popup {
    id: quickSwitcher

    property int textHeight: Math.round(Qt.application.font.pixelSize * 2.4)
    property int textMargin: Math.round(textHeight / 8)

    function delay(delayTime, cb) {
        timer.interval = delayTime;
        timer.repeat = false;
        timer.triggered.connect(cb);
        timer.start();
    }

    background: null
    width: Math.round(parent.width / 2)
    x: Math.round(parent.width / 2 - width / 2)
    y: Math.round(parent.height / 4 - height / 2)
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    parent: Overlay.overlay
    palette: colors
    onOpened: {
        completerPopup.open();
        delay(200, function() {
            roomTextInput.forceActiveFocus();
        });
    }
    onClosed: {
        completerPopup.close();
    }

    MatrixTextField {
        id: roomTextInput

        anchors.fill: parent
        font.pixelSize: Math.ceil(quickSwitcher.textHeight * 0.6)
        padding: textMargin
        color: colors.text
        onTextEdited: {
            completerPopup.completer.searchString = text;
        }
        Keys.onPressed: {
            if (event.key == Qt.Key_Up && completerPopup.opened) {
                event.accepted = true;
                completerPopup.up();
            } else if (event.key == Qt.Key_Down && completerPopup.opened) {
                event.accepted = true;
                completerPopup.down();
            } else if (event.matches(StandardKey.InsertParagraphSeparator)) {
                completerPopup.finishCompletion();
                event.accepted = true;
            }
        }
    }

    Completer {
        id: completerPopup

        x: roomTextInput.x
        y: roomTextInput.y + quickSwitcher.textHeight + quickSwitcher.textMargin
        visible: roomTextInput.length > 0
        width: parent.width
        completerName: "room"
        bottomToTop: false
        fullWidth: true
        avatarHeight: quickSwitcher.textHeight
        avatarWidth: quickSwitcher.textHeight
        centerRowContent: false
        rowMargin: Math.round(quickSwitcher.textMargin / 2)
        rowSpacing: quickSwitcher.textMargin
        closePolicy: Popup.NoAutoClose
    }

    Connections {
        onCompletionSelected: {
            TimelineManager.setHistoryView(id);
            TimelineManager.highlightRoom(id);
            quickSwitcher.close();
        }
        onCountChanged: {
            if (completerPopup.count > 0 && (completerPopup.currentIndex < 0 || completerPopup.currentIndex >= completerPopup.count))
                completerPopup.currentIndex = 0;

        }
        target: completerPopup
    }

    Timer {
        id: timer
    }

    Overlay.modal: Rectangle {
        color: "#aa1E1E1E"
    }

}
