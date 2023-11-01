// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./ui"
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import im.nheko 1.0

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
    property string roomId
    property int rowMargin: 0
    property int rowSpacing: Nheko.paddingSmall

    signal completionClicked(string completion)
    signal completionSelected(string id)

    function changeCompleter() {
        if (completerName) {
            completer = TimelineManager.completerFor(completerName, completerName == "room" ? "" : (popup.roomId != "" ? popup.roomId : room.roomId));
            completer.setSearchString("");
        } else {
            completer = undefined;
        }
        currentIndex = -1;
    }
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
        else if (popup.completerName == "user")
            popup.completionSelected(listView.itemAtIndex(currentIndex).modelData.userid);
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

    // Workaround palettes not inheriting for popups
    palette: timelineRoot.palette
    rightPadding: 1
    topPadding: 1

    background: Rectangle {
        border.color: palette.mid
        color: palette.base
    }
    contentItem: ListView {
        id: listView

        clip: true
        displayMarginBeginning: height / 2
        displayMarginEnd: height / 2
        highlightFollowsCurrentItem: true

        // If we have fewer than 7 items, just use the list view's content height.
        // Otherwise, we want to show 7 items.  Each item consists of row spacing between rows, row margins
        // on each side of a row, 1px of padding above the first item and below the last item, and nominally
        // some kind of content height.  avatarHeight is used for just about every delegate, so we're using
        // that until we find something better.  Put is all together and you have the formula below!
        implicitHeight: Math.min(contentHeight, 6 * rowSpacing + 7 * (popup.avatarHeight + 2 * rowMargin))

        // Broken, see https://bugreports.qt.io/browse/QTBUG-102811
        //reuseItems: true
        implicitWidth: Math.max(listView.contentItem.childrenRect.width, 20)
        model: completer
        pixelAligned: true
        spacing: rowSpacing
        verticalLayoutDirection: popup.bottomToTop ? ListView.BottomToTop : ListView.TopToBottom

        delegate: Rectangle {
            property variant modelData: model

            ListView.delayRemove: true
            color: model.index == popup.currentIndex ? palette.highlight : palette.base
            height: (chooser.child?.implicitHeight ?? 0) + 2 * popup.rowMargin
            implicitWidth: fullWidth ? ListView.view.width : chooser.child.implicitWidth + 4

            MouseArea {
                id: mouseArea

                anchors.fill: parent
                hoverEnabled: true

                onClicked: {
                    popup.completionClicked(completer.completionAt(model.index));
                    if (popup.completerName == "room")
                        popup.completionSelected(model.roomid);
                    else if (popup.completerName == "user")
                        popup.completionSelected(model.userid);
                }
                onPositionChanged: if (!listView.moving && !deadTimer.running)
                    popup.currentIndex = model.index
            }
            Ripple {
                color: Qt.rgba(palette.base.r, palette.base.g, palette.base.b, 0.5)
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
                        anchors.centerIn: centerRowContent ? parent : undefined
                        spacing: rowSpacing

                        Avatar {
                            displayName: model.displayName
                            enabled: false
                            Layout.preferredHeight: popup.avatarHeight
                            Layout.preferredWidth: popup.avatarWidth
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            userid: model.userid
                        }
                        Label {
                            color: model.index == popup.currentIndex ? palette.highlightedText : palette.text
                            text: model.displayName
                        }
                        Label {
                            color: model.index == popup.currentIndex ? palette.highlightedText : palette.buttonText
                            text: "(" + model.userid + ")"
                        }
                    }
                }
                DelegateChoice {
                    roleValue: "emoji"

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: rowSpacing

                        Label {
                            color: model.index == popup.currentIndex ? palette.highlightedText : palette.text
                            font: Settings.emojiFont
                            text: model.unicode
                            visible: !!model.unicode
                        }
                        Avatar {
                            crop: false
                            displayName: model.shortcode
                            enabled: false
                            Layout.preferredHeight: popup.avatarHeight
                            //userid: model.shortcode
                            url: (model.url ? model.url : "").replace("mxc://", "image://MxcImage/")
                            visible: !model.unicode
                            Layout.preferredWidth: popup.avatarWidth
                        }
                        Label {
                            Layout.leftMargin: Nheko.paddingSmall
                            Layout.rightMargin: Nheko.paddingSmall
                            color: model.index == popup.currentIndex ? palette.highlightedText : palette.text
                            text: model.shortcode
                        }
                        Label {
                            color: model.index == popup.currentIndex ? palette.highlightedText : palette.buttonText
                            text: "(" + model.packname + ")"
                        }
                    }
                }
                DelegateChoice {
                    roleValue: "command"

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: rowSpacing

                        Label {
                            color: model.index == popup.currentIndex ? palette.highlightedText : palette.text
                            font.bold: true
                            text: model.name
                        }
                        Label {
                            color: model.index == popup.currentIndex ? palette.highlightedText : palette.buttonText
                            text: model.description
                        }
                    }
                }
                DelegateChoice {
                    roleValue: "room"

                    RowLayout {
                        anchors.centerIn: centerRowContent ? parent : undefined
                        spacing: rowSpacing

                        Avatar {
                            displayName: model.roomName
                            enabled: false
                            Layout.preferredHeight: popup.avatarHeight
                            roomid: model.roomid
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            Layout.preferredWidth: popup.avatarWidth
                        }
                        Label {
                            color: model.index == popup.currentIndex ? palette.highlightedText : palette.text
                            font.italic: model.isTombstoned
                            font.bold: model.isSpace
                            font.pixelSize: popup.avatarHeight * 0.5
                            text: model.roomName
                            textFormat: Text.RichText
                        }
                    }
                }
                DelegateChoice {
                    roleValue: "roomAliases"

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: rowSpacing

                        Avatar {
                            displayName: model.roomName
                            enabled: false
                            Layout.preferredHeight: popup.avatarHeight
                            roomid: model.roomid
                            url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                            Layout.preferredWidth: popup.avatarWidth
                        }
                        Label {
                            color: model.index == popup.currentIndex ? palette.highlightedText : palette.text
                            font.italic: model.isTombstoned
                            font.bold: model.isSpace
                            text: model.roomName
                            textFormat: Text.RichText
                        }
                        Label {
                            color: model.index == popup.currentIndex ? palette.highlightedText : palette.buttonText
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

    onCompleterNameChanged: changeCompleter()
    onRoomIdChanged: changeCompleter()
}
