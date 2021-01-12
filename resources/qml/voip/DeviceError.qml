import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Popup {
    property string errorString
    property var image

    modal: true
    anchors.centerIn: parent

    RowLayout {
        Image {
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            source: "image://colorimage/" + image + "?" + colors.windowText
        }

        Label {
            text: errorString
            color: colors.windowText
        }

    }

    background: Rectangle {
        color: colors.window
        border.color: colors.windowText
    }

}
