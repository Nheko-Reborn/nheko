import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import im.nheko 1.0

Rectangle {
    id: activeCallBar

    visible: TimelineManager.callState != WebRTCState.DISCONNECTED
    color: "#2ECC71"
    implicitHeight: visible ? rowLayout.height + 8 : 0

    MouseArea {
        anchors.fill: parent
        onClicked: if (TimelineManager.onVideoCall)
                       stackLayout.currentIndex = stackLayout.currentIndex ? 0 : 1;
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
            url: TimelineManager.callPartyAvatarUrl.replace("mxc://", "image://MxcImage/")
            displayName: TimelineManager.callPartyName
        }

        Label {
            font.pointSize: fontMetrics.font.pointSize * 1.1
            text: "  " + TimelineManager.callPartyName + " "
        }

        Image {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            source: TimelineManager.onVideoCall ?
                        "qrc:/icons/icons/ui/video-call.png" : "qrc:/icons/icons/ui/place-call.png"
        }

        Label {
            id: callStateLabel

            font.pointSize: fontMetrics.font.pointSize * 1.1
        }

        Item {
            state: TimelineManager.callState
            states: [
                State {
                    name: "OFFERSENT"; when: state == WebRTCState.OFFERSENT
                    PropertyChanges { target: callStateLabel; text: "Calling..." }
                },
                State {
                    name: "CONNECTING"; when: state == WebRTCState.CONNECTING
                    PropertyChanges { target: callStateLabel; text: "Connecting..." }
                },
                State {
                    name: "ANSWERSENT"; when: state == WebRTCState.ANSWERSENT
                    PropertyChanges { target: callStateLabel; text: "Connecting..." }
                },
                State {
                    name: "CONNECTED"; when: state == WebRTCState.CONNECTED
                    PropertyChanges { target: callStateLabel; text: "00:00" }
                    PropertyChanges { target: callTimer; startTime: Math.floor((new Date()).getTime() / 1000) }
                    PropertyChanges { target: stackLayout; currentIndex: TimelineManager.onVideoCall ? 1 : 0 }
                },
                State {
                    name: "DISCONNECTED"; when: state == WebRTCState.DISCONNECTED
                    PropertyChanges { target: callStateLabel; text: "" }
                    PropertyChanges { target: stackLayout; currentIndex: 0 }
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
            running: TimelineManager.callState == WebRTCState.CONNECTED
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
            visible: TimelineManager.onVideoCall
            width: 24
            height: 24
            buttonTextColor: "#000000"
            image: ":/icons/icons/ui/toggle-camera-view.png"
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: "Toggle camera view"
            onClicked: TimelineManager.toggleCameraView()
        }

        Item {
            implicitWidth: 8
        }

        ImageButton {
            width: 24
            height: 24
            buttonTextColor: "#000000"
            image: TimelineManager.isMicMuted ? ":/icons/icons/ui/microphone-unmute.png" : ":/icons/icons/ui/microphone-mute.png"
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: TimelineManager.isMicMuted ? qsTr("Unmute Mic") : qsTr("Mute Mic")
            onClicked: TimelineManager.toggleMicMute()
        }

        Item {
            implicitWidth: 16
        }

    }

}
