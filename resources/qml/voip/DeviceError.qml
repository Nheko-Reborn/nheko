import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Popup {

    property string errorString
    property var iconSource

    modal: true
    anchors.centerIn: parent
    background: Rectangle {
        color: colors.window
        border.color: colors.windowText
    }

    RowLayout {

        Image {
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            source: iconSource
        }

        Label {
            text: errorString
            color: colors.windowText
        }
    }
}
