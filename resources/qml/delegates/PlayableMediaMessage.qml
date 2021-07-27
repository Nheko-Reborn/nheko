// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtMultimedia 5.6
import QtQuick 2.12
import QtQuick.Controls 2.1
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
                    source: media
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

                    positionText.text = formatTime(new Date(media.position));
                    durationText.text = formatTime(new Date(media.duration));
                }

                Layout.fillWidth: true
                value: media.position
                from: 0
                to: media.duration
                onMoved: media.seek(value)
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
                        room.cacheMedia(eventId);
                        break;
                    case "stopped":
                        media.play();
                        console.log("play");
                        button.state = "playing";
                        break;
                    case "playing":
                        media.pause();
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

                MediaPlayer {
                    id: media

                    onError: console.log(errorString)
                    onStatusChanged: {
                        if (status == MediaPlayer.Loaded)
                            progress.updatePositionTexts();

                    }
                    onStopped: button.state = "stopped"
                }

                Connections {
                    function onMediaCached(mxcUrl, cacheUrl) {
                        if (mxcUrl == url) {
                            media.source = cacheUrl;
                            button.state = "stopped";
                            console.log("media loaded: " + mxcUrl + " at " + cacheUrl);
                        }
                        console.log("media cached: " + mxcUrl + " at " + cacheUrl);
                    }

                    target: room
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
