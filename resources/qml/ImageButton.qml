import QtQuick 2.3
import QtQuick.Controls 2.3

AbstractButton {
    id: button

    property string image: undefined
    property color highlightColor: colors.highlight
    property color buttonTextColor: colors.buttonText

    width: 16
    height: 16

    Image {
        id: buttonImg

        // Workaround, can't get icon.source working for now...
        anchors.fill: parent
        source: "image://colorimage/" + image + "?" + (button.hovered ? highlightColor : buttonTextColor)
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        onPressed: mouse.accepted = false
        cursorShape: Qt.PointingHandCursor
    }

}
