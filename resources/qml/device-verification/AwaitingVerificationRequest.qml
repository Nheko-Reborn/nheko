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
			text: qsTr("Waiting for other side to accept the verification request.")
			color:colors.text
			verticalAlignment: Text.AlignVCenter
		}
		BusyIndicator {
			Layout.alignment: Qt.AlignHCenter
		}
		RowLayout {
			Button {
				Layout.alignment: Qt.AlignLeft
				text: qsTr("Cancel")
				palette {
                    button: "white"
                }
				contentItem: Text {
                    text: parent.text
                    color: "black"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
				onClicked: { 
					flow.cancelVerification(DeviceVerificationFlow.User);
					deviceVerificationList.remove(tran_id);
					dialog.destroy();
				}
			}
			Item {
				Layout.fillWidth: true
			}
		}
	}
}
