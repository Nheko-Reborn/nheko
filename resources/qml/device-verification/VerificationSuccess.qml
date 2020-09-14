import QtQuick 2.3
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.10

Pane {
	property string title: qsTr("Successful Verification")
	ColumnLayout {
		spacing: 16
		Label {
			Layout.maximumWidth: 400
			Layout.fillHeight: true
			Layout.fillWidth: true
			wrapMode: Text.Wrap
			id: content
			text: qsTr("Verification successful! Both sides verified their devices!")
			color:colors.text
			verticalAlignment: Text.AlignVCenter
		}
		RowLayout {
			Item {
				Layout.fillWidth: true
			}
			Button {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Close")
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
					if(flow) flow.deleteFlow();
					dialog.destroy();
				}
			}
		}
	}
}
