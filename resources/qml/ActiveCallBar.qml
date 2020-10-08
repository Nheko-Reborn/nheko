import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2

import im.nheko 1.0

Rectangle {
	id: activeCallBar
	visible: TimelineManager.callState != WebRTCState.DISCONNECTED
	color: "#2ECC71"
	implicitHeight: rowLayout.height + 8

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
			source: "qrc:/icons/icons/ui/place-call.png"
		}

		Label {
			id: callStateLabel
			font.pointSize: fontMetrics.font.pointSize * 1.1
		}

		Connections {
			target: TimelineManager
			function onCallStateChanged(state) {
				switch (state) {
					case WebRTCState.INITIATING:
						callStateLabel.text = qsTr("Initiating...")
						break;
					case WebRTCState.OFFERSENT:
						callStateLabel.text = qsTr("Calling...")
						break;
					case WebRTCState.CONNECTING:
						callStateLabel.text = qsTr("Connecting...")
						break;
					case WebRTCState.CONNECTED:
						callStateLabel.text = "00:00"
						var d = new Date()
						callTimer.startTime = Math.floor(d.getTime() / 1000)
						break;
					case WebRTCState.DISCONNECTED:
						callStateLabel.text = ""
				}
			}
		}

		Timer {
			id: callTimer
			property int startTime
			interval: 1000
			running: TimelineManager.callState == WebRTCState.CONNECTED
			repeat: true
			onTriggered: {
				var d = new Date()
				let seconds = Math.floor(d.getTime() / 1000 - startTime)
				let s = Math.floor(seconds % 60)
				let m = Math.floor(seconds / 60) % 60
				let h = Math.floor(seconds / 3600)
				callStateLabel.text = (h ? (pad(h) + ":") : "") + pad(m) + ":" + pad(s)
			}

			function pad(n) {
				return (n < 10) ? ("0" + n) : n
			}
		}

		Item {
			Layout.fillWidth: true
		}

		ImageButton {
			width: 24
			height: 24
			buttonTextColor: "#000000"
			image: TimelineManager.isMicMuted ?
				":/icons/icons/ui/microphone-unmute.png" :
				":/icons/icons/ui/microphone-mute.png"

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
