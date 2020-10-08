import QtQuick 2.3
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.10

import im.nheko 1.0

Pane {
	property string title: qsTr("Waiting for other party")
	ColumnLayout {
		spacing: 16
		Label {
			Layout.maximumWidth: 400
			Layout.fillHeight: true
			Layout.fillWidth: true
			wrapMode: Text.Wrap
			id: content
			text: switch (flow.state) {
				case "WaitingForOtherToAccept": return qsTr("Waiting for other side to accept the verification request.")
				case "WaitingForKeys": return qsTr("Waiting for other side to continue the verification request.")
				case "WaitingForMac": return qsTr("Waiting for other side to complete the verification request.")
			}

			color: colors.text
			verticalAlignment: Text.AlignVCenter
		}
		BusyIndicator {
			Layout.alignment: Qt.AlignHCenter
			palette: colors
		}
		RowLayout {
			Button {
				Layout.alignment: Qt.AlignLeft
				text: qsTr("Cancel")

				onClicked: { 
					flow.cancel();
					dialog.close();
				}
			}
			Item {
				Layout.fillWidth: true
			}
		}
	}
}
