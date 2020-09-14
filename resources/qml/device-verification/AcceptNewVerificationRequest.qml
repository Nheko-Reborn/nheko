import QtQuick 2.3
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.10

import im.nheko 1.0

Pane {
	property string title: qsTr("Recieving Device Verification Request")
	Component {
		id: awaitingVerificationRequestAccept
		AwaitingVerificationRequest {}
	}
	ColumnLayout {
		spacing: 16
		Label {
			Layout.maximumWidth: 400
			Layout.fillHeight: true
			Layout.fillWidth: true
			wrapMode: Text.Wrap
			text: qsTr("The device was requested to be verified")
			color:colors.text
			verticalAlignment: Text.AlignVCenter
		}
		RowLayout {
			Button {
				Layout.alignment: Qt.AlignLeft
				text: qsTr("Deny")
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
			Button {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Accept")
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
					stack.replace(awaitingVerificationRequestAccept); 
					isRequest?flow.sendVerificationReady():flow.acceptVerificationRequest(); 
				}
			}
		}
	}
}
