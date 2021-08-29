// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtMultimedia 5.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import im.nheko 1.0

Rectangle {
    id: bg

    required property double proportionalHeight
    required property int type
    required property int originalWidth
    required property string thumbnailUrl
    required property string eventId
    required property string url
    required property string body
    required property string filesize

    radius: 10
    color: Nheko.colors.alternateBase
    height: Math.round(content.height + 24)
    width: parent ? parent.width : undefined
    ListView.onPooled: height = 4
    ListView.onReused: height = Math.round(content.height + 24)

    Column {
        id: content

        width: parent.width - 24
        anchors.centerIn: parent

        Rectangle {
            id: videoContainer

            property double tempWidth: Math.min(parent ? parent.width : undefined, originalWidth < 1 ? 400 : originalWidth)
            property double tempHeight: tempWidth * proportionalHeight
            property double divisor: isReply ? 4 : 2
            property bool tooHigh: tempHeight > timelineView.height / divisor

            visible: type == MtxEvent.VideoMessage
            height: tooHigh ? timelineView.height / divisor : tempHeight
            width: tooHigh ? (timelineView.height / divisor) / proportionalHeight : tempWidth

            Image {
                anchors.fill: parent
                source: thumbnailUrl.replace("mxc://", "image://MxcImage/")
                asynchronous: true
                fillMode: Image.PreserveAspectFit

                VideoOutput {
                    anchors.fill: parent
                    fillMode: VideoOutput.PreserveAspectFit
                    flushMode: VideoOutput.FirstFrame
                    source: mxcmedia
                }

            }

        }

        RowLayout {
            width: parent.width

            Text {
                id: positionText

                text: "--:--:--"
                color: Nheko.colors.text
            }

            Slider {
                id: progress

                //indeterminate: true
                function updatePositionTexts() {
                    function formatTime(date) {
                        var hh = date.getUTCHours();
                        var mm = date.getUTCMinutes();
                        var ss = date.getSeconds();
                        if (hh < 10)
                            hh = "0" + hh;

                        if (mm < 10)
                            mm = "0" + mm;

                        if (ss < 10)
                            ss = "0" + ss;

                        return hh + ":" + mm + ":" + ss;
                    }

                    positionText.text = formatTime(new Date(mxcmedia.position));
                    durationText.text = formatTime(new Date(mxcmedia.duration));
                }

                Layout.fillWidth: true
                value: mxcmedia.position
                from: 0
                to: mxcmedia.duration
                onMoved: mxcmedia.position = value
                onValueChanged: updatePositionTexts()
                palette: Nheko.colors
            }

            Text {
                id: durationText

                text: "--:--:--"
                color: Nheko.colors.text
            }

        }

        RowLayout {
            width: parent.width
            spacing: 15

            ImageButton {
                id: button

                Layout.alignment: Qt.AlignVCenter
                //color: Nheko.colors.window
                //radius: 22
                height: 32
                width: 32
                z: 3
                image: ":/icons/icons/ui/arrow-pointing-down.png"
                onClicked: {
                    switch (button.state) {
                    case "":
                        mxcmedia.eventId = eventId;
                        break;
                    case "stopped":
                        mxcmedia.play();
                        console.log("play");
                        button.state = "playing";
                        break;
                    case "playing":
                        mxcmedia.pause();
                        console.log("pause");
                        button.state = "stopped";
                        break;
                    }
                }
                states: [
                    State {
                        name: "stopped"

                        PropertyChanges {
                            target: button
                            image: ":/icons/icons/ui/play-sign.png"
                        }

                    },
                    State {
                        name: "playing"

                        PropertyChanges {
                            target: button
                            image: ":/icons/icons/ui/pause-symbol.png"
                        }

                    }
                ]

                CursorShape {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                }

                MxcMedia {
                    id: mxcmedia

                    roomm: room
                    onError: console.log(errorString)
                    onMediaStatusChanged: {
                        if (status == MxcMedia.LoadedMedia) {
                            progress.updatePositionTexts();
                            button.state = "stopped";
                        }
                    }
                    onStateChanged: {
                        if (state == MxcMedia.StoppedState)
                            button.state = "stopped";

                    }
                }

            }

            ColumnLayout {
                id: col

                Text {
                    Layout.fillWidth: true
                    text: body
                    elide: Text.ElideRight
                    color: Nheko.colors.text
                }

                Text {
                    Layout.fillWidth: true
                    text: filesize
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                    color: Nheko.colors.text
                }

            }

        }

    }

}
