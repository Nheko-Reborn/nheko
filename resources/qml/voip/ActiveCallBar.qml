// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Rectangle {
    visible: CallManager.isOnCall
    color: callInviteBar.color
    implicitHeight: visible ? rowLayout.height + 8 : 0

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (CallManager.callType != Voip.VOICE)
                stackLayout.currentIndex = stackLayout.currentIndex ? 0 : 1;

        }
    }

    RowLayout {
        id: rowLayout

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 8

        Avatar {
            implicitWidth: Nheko.avatarSize
            implicitHeight: Nheko.avatarSize
            url: CallManager.callPartyAvatarUrl.replace("mxc://", "image://MxcImage/")
            userid: CallManager.callParty
            displayName: CallManager.callPartyDisplayName
            onClicked: TimelineManager.openImageOverlay(room, room.avatarUrl(userid), room.data.eventId)
        }

        Label {
            Layout.leftMargin: 8
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: CallManager.callPartyDisplayName
            color: "#000000"
        }

        Image {
            id: callTypeIcon

            Layout.leftMargin: 4
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
        }

        Item {
            states: [
                State {
                    name: "VOICE"
                    when: CallManager.callType == Voip.VOICE

                    PropertyChanges {
                        callTypeIcon.source: "qrc:/icons/icons/ui/place-call.svg"
                    }

                },
                State {
                    name: "VIDEO"
                    when: CallManager.callType == Voip.VIDEO

                    PropertyChanges {
                        callTypeIcon.source: "qrc:/icons/icons/ui/video.svg"
                    }

                },
                State {
                    name: "SCREEN"
                    when: CallManager.callType == Voip.SCREEN

                    PropertyChanges {
                        callTypeIcon.source: "qrc:/icons/icons/ui/screen-share.svg"
                    }

                }
            ]
        }

        Label {
            id: callStateLabel

            font.pointSize: fontMetrics.font.pointSize * 1.1
            color: "#000000"
        }

        Item {
            states: [
                State {
                    name: "OFFERSENT"
                    when: CallManager.callState == Voip.OFFERSENT

                    PropertyChanges {
                        callStateLabel.text: qsTr("Calling...")
                    }

                },
                State {
                    name: "CONNECTING"
                    when: CallManager.callState == Voip.CONNECTING

                    PropertyChanges {
                        callStateLabel.text: qsTr("Connecting...")
                    }

                },
                State {
                    name: "ANSWERSENT"
                    when: CallManager.callState == Voip.ANSWERSENT

                    PropertyChanges {
                        callStateLabel.text: qsTr("Connecting...")
                    }

                },
                State {
                    name: "CONNECTED"
                    when: CallManager.callState == Voip.CONNECTED

                    PropertyChanges {
                        callStateLabel.text: "00:00"
                    }

                    PropertyChanges {
                        callTimer.startTime: Math.floor((new Date()).getTime() / 1000)
                    }

                    // HACK(Nico): Somehow this causes a crash when not using the custom parser for that property change...
                    //PropertyChanges {
                    //    stackLayout.currentIndex: CallManager.callType != Voip.VOICE ? 1 : 0
                    //}

                    PropertyChanges {
                        target: stackLayout
                        currentIndex: CallManager.callType != Voip.VOICE ? 1 : 0 // qmllint disable Quick.property-changes-parsed
                    }

                },
                State {
                    name: "DISCONNECTED"
                    when: CallManager.callState == Voip.DISCONNECTED

                    PropertyChanges {
                        callStateLabel.text: ""
                    }

                    // HACK(Nico): Somehow this causes a crash when not using the custom parser for that property change...
                    //PropertyChanges {
                    //    stackLayout.currentIndex: 0
                    //}
                    PropertyChanges {
                        target: stackLayout
                        currentIndex: 0 // qmllint disable Quick.property-changes-parsed
                    }

                }
            ]
        }

        Timer {
            id: callTimer

            property int startTime

            function pad(n) {
                return (n < 10) ? ("0" + n) : n;
            }

            interval: 1000
            running: CallManager.callState == Voip.CONNECTED
            repeat: true
            onTriggered: {
                var d = new Date();
                let seconds = Math.floor(d.getTime() / 1000 - startTime);
                let s = Math.floor(seconds % 60);
                let m = Math.floor(seconds / 60) % 60;
                let h = Math.floor(seconds / 3600);
                callStateLabel.text = (h ? (pad(h) + ":") : "") + pad(m) + ":" + pad(s);
            }
        }

        Label {
            Layout.leftMargin: 16
            visible: CallManager.callType == Voip.SCREEN && CallManager.callState == Voip.CONNECTED
            text: qsTr("You are screen sharing")
            font.pointSize: fontMetrics.font.pointSize * 1.1
            color: "#000000"
        }

        Item {
            Layout.fillWidth: true
        }

        ImageButton {
            visible: CallManager.haveLocalPiP
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            buttonTextColor: "#000000"
            image: ":/icons/icons/ui/picture-in-picture.svg"
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Hide/Show Picture-in-Picture")
            onClicked: CallManager.toggleLocalPiP()
        }

        ImageButton {
            Layout.leftMargin: 8
            Layout.rightMargin: 16
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            buttonTextColor: "#000000"
            image: CallManager.isMicMuted ? ":/icons/icons/ui/microphone-unmute.svg" : ":/icons/icons/ui/microphone-mute.svg"
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: CallManager.isMicMuted ? qsTr("Unmute Mic") : qsTr("Mute Mic")
            onClicked: CallManager.toggleMicMute()
        }

    }

}
