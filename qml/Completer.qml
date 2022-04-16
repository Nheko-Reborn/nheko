// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import im.nheko
import im.nheko
import "ui"

Control {
    id: popup

    property int avatarHeight: 24
    property int avatarWidth: 24
    property bool bottomToTop: true
    property bool centerRowContent: true
    property var completer
    property string completerName
    property alias count: listView.count
    property alias currentIndex: listView.currentIndex
    property bool fullWidth: false
    property int rowMargin: 0
    property int rowSpacing: 5

    signal completionClicked(string completion)
    signal completionSelected(string id)

    function currentCompletion() {
        if (currentIndex > -1 && currentIndex < listView.count)
            return completer.completionAt(currentIndex);
        else
            return null;
    }
    function down() {
        if (bottomToTop)
            up_();
        else
            down_();
    }
    function down_() {
        currentIndex = currentIndex + 1;
        if (currentIndex >= listView.count)
            currentIndex = -1;
    }
    function finishCompletion() {
        if (popup.completerName == "room")
            popup.completionSelected(listView.itemAtIndex(currentIndex).modelData.roomid);
    }
    function up() {
        if (bottomToTop)
            down_();
        else
            up_();
    }
    function up_() {
        currentIndex = currentIndex - 1;
        if (currentIndex == -2)
            currentIndex = listView.count - 1;
    }

    bottomPadding: 1
    leftPadding: 1
    rightPadding: 1
    topPadding: 1

    background: Rectangle {
        border.color: timelineRoot.palette.mid
        color: timelineRoot.palette.base
    }
    contentItem: ListView {
        id: listView
        clip: true
        highlightFollowsCurrentItem: true

        // If we have fewer than 7 items, just use the list view's content height.
        // Otherwise, we want to show 7 items.  Each item consists of row spacing between rows, row margins
        // on each side of a row, 1px of padding above the first item and below the last item, and nominally
        // some kind of content height.  avatarHeight is used for just about every delegate, so we're using
        // that until we find something better.  Put is all together and you have the formula below!
        implicitHeight: Math.min(contentHeight, 6 * rowSpacing + 7 * (popup.avatarHeight + 2 * rowMargin))
        implicitWidth: listView.contentItem.childrenRect.width
        model: completer
        pixelAligned: true
        reuseItems: true
        spacing: rowSpacing
        verticalLayoutDirection: popup.bottomToTop ? ListView.BottomToTop : ListView.TopToBottom

        delegate: Rectangle {
            property variant modelData: model

            color: model.index == popup.currentIndex ? timelineRoot.palette.highlight : timelineRoot.palette.base
            height: chooser.child.implicitHeight + 2 * popup.rowMargin
            implicitWidth: fullWidth ? ListView.view.width : chooser.child.implicitWidth + 4

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true

                onClicked: {
                    popup.completionClicked(completer.completionAt(model.index));
                    if (popup.completerName == "room")
                        popup.completionSelected(model.roomid);
                }
                onPositionChanged: if (!listView.moving && !deadTimer.running)
                    popup.currentIndex = model.index
            }
            Ripple {
                color: Qt.rgba(timelineRoot.palette.base.r, timelineRoot.palette.base.g, timelineRoot.palette.base.b, 0.5)
            }
            DelegateChooser {
                id: chooser
                anchors.fill: parent
                anchors.margins: popup.rowMargin
                enabled: false
                roleValue: popup.completerName

                DelegateChoice {
                    roleValue: "user"

                    RowLayout {
                        id: del
                        anchors.centerIn: parent
                        spacing: rowSpacing

                        Avatar {
                            displayName: model.displayName
                            height: popup.avatarHeight
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            userid: model.userid
                            width: popup.avatarWidth

                            onClicked: popup.completionClicked(completer.completionAt(model.index))
                        }
                        Label {
                            color: model.index == popup.currentIndex ? timelineRoot.palette.highlightedText : timelineRoot.palette.text
                            text: model.displayName
                        }
                        Label {
                            color: model.index == popup.currentIndex ? timelineRoot.palette.highlightedText : timelineRoot.palette.placeholderText
                            text: "(" + model.userid + ")"
                        }
                    }
                }
                DelegateChoice {
                    roleValue: "emoji"

                    RowLayout {
                        id: del
                        anchors.centerIn: parent
                        spacing: rowSpacing

                        Label {
                            color: model.index == popup.currentIndex ? timelineRoot.palette.highlightedText : timelineRoot.palette.text
                            font: Settings.emojiFont
                            text: model.unicode
                        }
                        Label {
                            color: model.index == popup.currentIndex ? timelineRoot.palette.highlightedText : timelineRoot.palette.text
                            text: model.shortName
                        }
                    }
                }
                DelegateChoice {
                    roleValue: "customEmoji"

                    RowLayout {
                        id: del
                        anchors.centerIn: parent
                        spacing: rowSpacing

                        Avatar {
                            crop: false
                            displayName: model.shortcode
                            height: popup.avatarHeight
                            //userid: model.shortcode
                            url: model.url.replace("mxc://", "image://MxcImage/")
                            width: popup.avatarWidth

                            onClicked: popup.completionClicked(completer.completionAt(model.index))
                        }
                        Label {
                            color: model.index == popup.currentIndex ? timelineRoot.palette.highlightedText : timelineRoot.palette.text
                            text: model.shortcode
                        }
                        Label {
                            color: model.index == popup.currentIndex ? timelineRoot.palette.highlightedText : timelineRoot.palette.placeholderText
                            text: "(" + model.packname + ")"
                        }
                    }
                }
                DelegateChoice {
                    roleValue: "room"

                    RowLayout {
                        id: del
                        anchors.centerIn: centerRowContent ? parent : undefined
                        spacing: rowSpacing

                        Avatar {
                            displayName: model.roomName
                            height: popup.avatarHeight
                            roomid: model.roomid
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            width: popup.avatarWidth

                            onClicked: {
                                popup.completionClicked(completer.completionAt(model.index));
                                popup.completionSelected(model.roomid);
                            }
                        }
                        Label {
                            color: model.index == popup.currentIndex ? timelineRoot.palette.highlightedText : timelineRoot.palette.text
                            font.pixelSize: popup.avatarHeight * 0.5
                            text: model.roomName
                            textFormat: Text.RichText
                        }
                    }
                }
                DelegateChoice {
                    roleValue: "roomAliases"

                    RowLayout {
                        id: del
                        anchors.centerIn: parent
                        spacing: rowSpacing

                        Avatar {
                            displayName: model.roomName
                            height: popup.avatarHeight
                            roomid: model.roomid
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            width: popup.avatarWidth

                            onClicked: popup.completionClicked(completer.completionAt(model.index))
                        }
                        Label {
                            color: model.index == popup.currentIndex ? timelineRoot.palette.highlightedText : timelineRoot.palette.text
                            text: model.roomName
                            textFormat: Text.RichText
                        }
                        Label {
                            color: model.index == popup.currentIndex ? timelineRoot.palette.highlightedText : timelineRoot.palette.placeholderText
                            text: "(" + model.roomAlias + ")"
                            textFormat: Text.RichText
                        }
                    }
                }
            }
        }

        onContentYChanged: deadTimer.restart()

        Timer {
            id: deadTimer
            interval: 50
        }
    }

    onCompleterNameChanged: {
        if (completerName) {
            completer = TimelineManager.completerFor(completerName, completerName == "room" ? "" : room.roomId);
            completer.setSearchString("");
        } else {
            completer = undefined;
        }
        currentIndex = -1;
    }
}
