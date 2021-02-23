import QtQuick 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.13

TextInput {
    id: input

    Rectangle {
        id: blueBar

        anchors.top: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        color: colors.highlight
        height: 1
        width: parent.width
    
        Rectangle {
            id: blackBar

            anchors.verticalCenter: blueBar.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter

            height: parent.height+1
            width: 0
            color: colors.text
        
            states: State {
                name: "focused"; when: input.activeFocus == true
                PropertyChanges {
                    target: blackBar
                    width: blueBar.width
                }
            }
        
            transitions: Transition {
                from: ""
                to: "focused"
                reversible: true

                NumberAnimation { 
                    target: blackBar
                    properties: "width"
                    duration: 500
                    easing.type: Easing.InOutQuad
                    alwaysRunToEnd: true 
                }
            }
        }
    }
}