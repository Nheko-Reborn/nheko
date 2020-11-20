import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Popup {
    id: popup

    property int currentIndex: -1
    property string completerName
    property var completer

    function up() {
        currentIndex = currentIndex - 1;
        if (currentIndex == -2)
            currentIndex = repeater.count - 1;

    }

    function down() {
        currentIndex = currentIndex + 1;
        if (currentIndex >= repeater.count)
            currentIndex = -1;

    }

    function currentCompletion() {
        if (currentIndex > -1 && currentIndex < repeater.count)
            return completer.completionAt(currentIndex);
        else
            return null;
    }

    onCompleterNameChanged: {
        if (completerName)
            completer = TimelineManager.timeline.input.completerFor(completerName);
        else
            completer = undefined;
    }
    padding: 0
    onAboutToShow: currentIndex = -1

    Connections {
        onTimelineChanged: completer = null
        target: TimelineManager
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Repeater {
            id: repeater

            model: completer

            delegate: Rectangle {
                color: model.index == popup.currentIndex ? colors.window : colors.base
                height: del.implicitHeight + 4
                width: del.implicitWidth + 4

                RowLayout {
                    id: del

                    anchors.centerIn: parent

                    Avatar {
                        height: 24
                        width: 24
                        displayName: model.displayName
                        url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                    }

                    Label {
                        text: model.displayName
                        color: colors.text
                    }

                }

            }

        }

    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: 100
        }

    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: 100
        }

    }

    background: Rectangle {
        color: colors.base
        implicitHeight: popup.contentHeight
        implicitWidth: popup.contentWidth
    }

}
