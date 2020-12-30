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
            if (CallManager.isVideo)
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
            width: avatarSize
            height: avatarSize
            url: CallManager.callPartyAvatarUrl.replace("mxc://", "image://MxcImage/")
            displayName: CallManager.callParty
        }

        Label {
            Layout.leftMargin: 8
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: CallManager.callParty
            color: "#000000"
        }

        Image {
            Layout.leftMargin: 4
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            source: CallManager.isVideo ? "qrc:/icons/icons/ui/video-call.png" : "qrc:/icons/icons/ui/place-call.png"
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
                    when: CallManager.callState == WebRTCState.OFFERSENT

                    PropertyChanges {
                        target: callStateLabel
                        text: qsTr("Calling...")
                    }

                },
                State {
                    name: "CONNECTING"
                    when: CallManager.callState == WebRTCState.CONNECTING

                    PropertyChanges {
                        target: callStateLabel
                        text: qsTr("Connecting...")
                    }

                },
                State {
                    name: "ANSWERSENT"
                    when: CallManager.callState == WebRTCState.ANSWERSENT

                    PropertyChanges {
                        target: callStateLabel
                        text: qsTr("Connecting...")
                    }

                },
                State {
                    name: "CONNECTED"
                    when: CallManager.callState == WebRTCState.CONNECTED

                    PropertyChanges {
                        target: callStateLabel
                        text: "00:00"
                    }

                    PropertyChanges {
                        target: callTimer
                        startTime: Math.floor((new Date()).getTime() / 1000)
                    }

                    PropertyChanges {
                        target: stackLayout
                        currentIndex: CallManager.isVideo ? 1 : 0
                    }

                },
                State {
                    name: "DISCONNECTED"
                    when: CallManager.callState == WebRTCState.DISCONNECTED

                    PropertyChanges {
                        target: callStateLabel
                        text: ""
                    }

                    PropertyChanges {
                        target: stackLayout
                        currentIndex: 0
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
            running: CallManager.callState == WebRTCState.CONNECTED
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

        Item {
            Layout.fillWidth: true
        }

        ImageButton {
            visible: CallManager.haveLocalVideo
            width: 24
            height: 24
            buttonTextColor: "#000000"
            image: ":/icons/icons/ui/toggle-camera-view.png"
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Toggle camera view")
            onClicked: CallManager.toggleCameraView()
        }

        ImageButton {
            Layout.leftMargin: 8
            Layout.rightMargin: 16
            width: 24
            height: 24
            buttonTextColor: "#000000"
            image: CallManager.isMicMuted ? ":/icons/icons/ui/microphone-unmute.png" : ":/icons/icons/ui/microphone-mute.png"
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: CallManager.isMicMuted ? qsTr("Unmute Mic") : qsTr("Mute Mic")
            onClicked: CallManager.toggleMicMute()
        }
    }
}
