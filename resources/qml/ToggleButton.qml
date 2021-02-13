import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import im.nheko 1.0

Switch {
    style: SwitchStyle {
        handle: Rectangle {
            width: 20
            height: 20
            radius: 90
            color: "whitesmoke"
        }
        
        groove: Rectangle {
            implicitWidth: 40
            implicitHeight: 20
            radius: 90
            color: checked ? "skyblue" : "grey"
        }
    }
}