import "./ui"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Popup {
    id: popup

    property int currentIndex: -1
    property string completerName
    property var completer
    property bool bottomToTop: true
    property bool fullWidth: false
    property alias count: listView.count

    signal completionClicked(string completion)
    signal completionSelected(string id)

    function up() {
        if (bottomToTop)
            down_();
        else
            up_();
    }

    function down() {
        if (bottomToTop)
            up_();
        else
            down_();
    }

    function up_() {
        currentIndex = currentIndex - 1;
        if (currentIndex == -2)
            currentIndex = listView.count - 1;

    }

    function down_() {
        currentIndex = currentIndex + 1;
        if (currentIndex >= listView.count)
            currentIndex = -1;

    }

    function currentCompletion() {
        if (currentIndex > -1 && currentIndex < listView.count)
            return completer.completionAt(currentIndex);
        else
            return null;
    }

    function finishCompletion() {
        if(popup.completerName == "room") {
            popup.completionSelected(listView.itemAtIndex(currentIndex).modelData.roomid)
        }
    }

    onCompleterNameChanged: {
        if (completerName) {
            if (completerName == "user") {
                completer = TimelineManager.completerFor(completerName, TimelineManager.timeline.roomId());
            } else {
                completer = TimelineManager.completerFor(completerName);
            }
            completer.setSearchString("");
        } else {
            completer = undefined;
        }
    }
    padding: 1
    onAboutToShow: currentIndex = -1
    height: listView.contentHeight + 2 // + 2 for the padding on top and bottom

    Connections {
        onTimelineChanged: completer = null
        target: TimelineManager
    }

    ListView {
        id: listView

        anchors.fill: parent
        implicitWidth: fullWidth ? parent.width : contentItem.childrenRect.width
        model: completer
        verticalLayoutDirection: popup.bottomToTop ? ListView.BottomToTop : ListView.TopToBottom

        delegate: Rectangle {
            color: model.index == popup.currentIndex ? colors.highlight : colors.base
            height: chooser.childrenRect.height + 4
            implicitWidth: fullWidth ? popup.width : chooser.childrenRect.width + 4
            property variant modelData: model

            MouseArea {
                id: mouseArea

                anchors.fill: parent
                hoverEnabled: true
                onPositionChanged: popup.currentIndex = model.index
                onClicked: {
                    popup.completionClicked(completer.completionAt(model.index))
                    if(popup.completerName == "room") {
                        popup.completionSelected(model.roomid)
                    }
                }

                Ripple {
                    rippleTarget: mouseArea
                    color: Qt.rgba(colors.base.r, colors.base.g, colors.base.b, 0.5)
                }

            }

            DelegateChooser {
                id: chooser

                roleValue: popup.completerName
                anchors.centerIn: parent

                DelegateChoice {
                    roleValue: "user"

                    RowLayout {
                        id: del

                        anchors.centerIn: parent

                        Avatar {
                            height: 24
                            width: 24
                            displayName: model.displayName
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            onClicked: popup.completionClicked(completer.completionAt(model.index))
                        }

                        Label {
                            text: model.displayName
                            color: model.index == popup.currentIndex ? colors.highlightedText : colors.text
                        }

                        Label {
                            text: "(" + model.userid + ")"
                            color: model.index == popup.currentIndex ? colors.highlightedText : colors.buttonText
                        }

                    }

                }

                DelegateChoice {
                    roleValue: "emoji"

                    RowLayout {
                        id: del

                        anchors.centerIn: parent

                        Label {
                            text: model.unicode
                            color: model.index == popup.currentIndex ? colors.highlightedText : colors.text
                            font: Settings.emojiFont
                        }

                        Label {
                            text: model.shortName
                            color: model.index == popup.currentIndex ? colors.highlightedText : colors.text
                        }

                    }

                }

                DelegateChoice {
                    roleValue: "room"

                    RowLayout {
                        id: del

                        anchors.centerIn: parent

                        Avatar {
                            height: 24
                            width: 24
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            onClicked: {
                                popup.completionClicked(completer.completionAt(model.index))
                                popup.completionSelected(model.roomid)
                            }
                        }

                        Label {
                            text: model.roomName
                            color: model.index == popup.currentIndex ? colors.highlightedText : colors.text
                        }

                    }

                }

                DelegateChoice {
                    roleValue: "roomAliases"

                    RowLayout {
                        id: del

                        anchors.centerIn: parent

                        Avatar {
                            height: 24
                            width: 24
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            onClicked: popup.completionClicked(completer.completionAt(model.index))
                        }

                        Label {
                            text: model.roomName
                            color: model.index == popup.currentIndex ? colors.highlightedText : colors.text
                        }

                        Label {
                            text: "(" + model.roomAlias + ")"
                            color: model.index == popup.currentIndex ? colors.highlightedText : colors.buttonText
                        }

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
        border.color: colors.mid
    }

}
