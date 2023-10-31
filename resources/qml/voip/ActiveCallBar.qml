// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../"
import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Rectangle {
    color: callInviteBar.color
    implicitHeight: visible ? rowLayout.height + 8 : 0
    visible: CallManager.isOnCall

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
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        Avatar {
            displayName: CallManager.callPartyDisplayName
            implicitHeight: Nheko.avatarSize
            implicitWidth: Nheko.avatarSize
            url: CallManager.callPartyAvatarUrl.replace("mxc://", "image://MxcImage/")
            userid: CallManager.callParty

            onClicked: TimelineManager.openImageOverlay(room, room.avatarUrl(userid), room.data.eventId)
        }
        Label {
            Layout.leftMargin: 8
            color: "#000000"
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: CallManager.callPartyDisplayName
        }
        Image {
            id: callTypeIcon

            Layout.leftMargin: 4
            Layout.preferredHeight: 24
            Layout.preferredWidth: 24
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

            color: "#000000"
            font.pointSize: fontMetrics.font.pointSize * 1.1
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
                    PropertyChanges {
                        stackLayout.currentIndex: CallManager.callType != Voip.VOICE ? 1 : 0
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
                        currentIndex: 0 // qmllint disable Quick.property-changes-parsed
                        target: stackLayout
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
            repeat: true
            running: CallManager.callState == Voip.CONNECTED

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
            color: "#000000"
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: qsTr("You are screen sharing")
            visible: CallManager.callType == Voip.SCREEN && CallManager.callState == Voip.CONNECTED
        }
        Item {
            Layout.fillWidth: true
        }
        ImageButton {
            Layout.preferredHeight: 24
            Layout.preferredWidth: 24
            ToolTip.text: qsTr("Hide/Show Picture-in-Picture")
            ToolTip.visible: hovered
            buttonTextColor: "#000000"
            hoverEnabled: true
            image: ":/icons/icons/ui/picture-in-picture.svg"
            visible: CallManager.haveLocalPiP

            onClicked: CallManager.toggleLocalPiP()
        }
        ImageButton {
            Layout.leftMargin: 8
            Layout.preferredHeight: 24
            Layout.preferredWidth: 24
            Layout.rightMargin: 16
            ToolTip.text: CallManager.isMicMuted ? qsTr("Unmute Mic") : qsTr("Mute Mic")
            ToolTip.visible: hovered
            buttonTextColor: "#000000"
            hoverEnabled: true
            image: CallManager.isMicMuted ? ":/icons/icons/ui/microphone-unmute.svg" : ":/icons/icons/ui/microphone-mute.svg"

            onClicked: CallManager.toggleMicMute()
        }
    }
}
