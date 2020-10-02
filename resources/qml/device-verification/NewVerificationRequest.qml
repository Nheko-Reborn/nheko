import QtQuick 2.3
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.10

Pane {
	property string title: qsTr("Sending Device Verification Request")
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
			text: qsTr("A new device was added.")
			color:colors.text
			verticalAlignment: Text.AlignVCenter
		}
		Label {
			Layout.maximumWidth: 400
			Layout.fillHeight: true
			Layout.fillWidth: true
			wrapMode: Text.Wrap
			text: qsTr("The device may have been added by you signing in from another client or physical device. To ensure that no malicious user can eavesdrop on your encrypted communications, you should verify the new device.")
			color:colors.text
			verticalAlignment: Text.AlignVCenter
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
					deviceVerificationList.remove(tran_id);
					flow.deleteFlow();
					dialog.destroy();  
				}
			}
			Item {
				Layout.fillWidth: true
			}
			Button {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start verification")
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
					flow.sender ?flow.sendVerificationRequest():flow.startVerificationRequest(); }
			}
		}
	}
}
