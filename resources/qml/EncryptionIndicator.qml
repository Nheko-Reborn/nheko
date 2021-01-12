import QtQuick 2.5
import QtQuick.Controls 2.1
import im.nheko 1.0

Rectangle {
    id: indicator

    property bool encrypted: false

    function getEncryptionImage() {
        if (encrypted)
            return "image://colorimage/:/icons/icons/ui/lock.png?" + colors.buttonText;
        else
            return "image://colorimage/:/icons/icons/ui/unlock.png?#dd3d3d";
    }

    function getEncryptionTooltip() {
        if (encrypted)
            return qsTr("Encrypted");
        else
            return qsTr("This message is not encrypted!");
    }

    color: "transparent"
    width: 16
    height: 16
    ToolTip.visible: ma.containsMouse && indicator.visible
    ToolTip.text: getEncryptionTooltip()

    MouseArea {
        id: ma

        anchors.fill: parent
        hoverEnabled: true
    }

    Image {
        id: stateImg

        anchors.fill: parent
        source: getEncryptionImage()
    }

}
