import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import QtQuick.Window 2.3

import im.nheko 1.0

import "./device-verification"

ApplicationWindow{
	property var profile

	id: userProfileDialog
	height: 650
	width: 420
	modality: Qt.WindowModal
	Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
	palette: colors

	Connections{
		target: deviceVerificationList
		onUpdateProfile: {
			profile.fetchDeviceList(profile.userid)
		}
	}

	Component {
		id: deviceVerificationDialog
		DeviceVerification {}
	}
	Component{
		id: deviceVerificationFlow
		DeviceVerificationFlow {}
	}

	Item{
		id: userProfileItem
		width: userProfileDialog.width
		height: userProfileDialog.height

		// Layout.fillHeight : true

		ColumnLayout{
			anchors.fill: userProfileItem
			width: userProfileDialog.width
			spacing: 10

			Avatar {
				url: profile.avatarUrl.replace("mxc://", "image://MxcImage/")
				height: 130
				width: 130
				displayName: profile.displayName
				userid: profile.userid
				Layout.alignment: Qt.AlignHCenter
				Layout.margins : {
					top: 10
				}
			}

			Label {
				text: profile.displayName
				fontSizeMode: Text.HorizontalFit
				font.pixelSize: 20
				color: TimelineManager.userColor(profile.userid, colors.window)
				font.bold: true
				Layout.alignment: Qt.AlignHCenter
			}

			TextEdit {
				text: profile.userid
				selectByMouse: true
				font.pixelSize: 15
				color: colors.text
				Layout.alignment: Qt.AlignHCenter
			}

			Button {
				id: verifyUserButton
				text: "Verify"
				Layout.alignment: Qt.AlignHCenter
				enabled: profile.isUserVerified?false:true
				visible: profile.isUserVerified?false:true
				palette {
					button: "white"
				}
				contentItem: Text {
					text: verifyUserButton.text
					color: "black"
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
				}
				onClicked: {
					var newFlow = profile.createFlow(true);
					newFlow.userId = profile.userid;
					newFlow.sender = true;
					deviceVerificationList.add(newFlow.tranId);
					var dialog = deviceVerificationDialog.createObject(userProfileDialog, {flow: newFlow,isRequest: true,tran_id: newFlow.tranId});
					dialog.show();
				}
			}

			RowLayout {
				Layout.alignment: Qt.AlignHCenter
				ImageButton {
					image:":/icons/icons/ui/do-not-disturb-rounded-sign.png"
					Layout.margins: {
						left: 5
						right: 5
					}
					ToolTip.visible: hovered
					ToolTip.text: qsTr("Ban the user")
					onClicked : {
						profile.banUser()
					}
				}
				// ImageButton{
				//     image:":/icons/icons/ui/volume-off-indicator.png"
				//     Layout.margins: {
				//         left: 5
				//         right: 5
				//     }
				//     ToolTip.visible: hovered
				//     ToolTip.text: qsTr("Ignore messages from this user")
				//     onClicked : {
				//         profile.ignoreUser()
				//     }
				// }
				ImageButton{
					image:":/icons/icons/ui/black-bubble-speech.png"
					Layout.margins: {
						left: 5
						right: 5
					}
					ToolTip.visible: hovered
					ToolTip.text: qsTr("Start a private chat")
					onClicked : {
						profile.startChat()
					}
				}
				ImageButton{
					image:":/icons/icons/ui/round-remove-button.png"
					Layout.margins: {
						left: 5
						right: 5
					}
					ToolTip.visible: hovered
					ToolTip.text: qsTr("Kick the user")
					onClicked : {
						profile.kickUser()
					}
				}
			}

			ScrollView {
				implicitHeight: userProfileDialog.height/2-13
				implicitWidth: userProfileDialog.width-20
				clip: true
				Layout.alignment: Qt.AlignHCenter

				ListView{
					id: devicelist
					anchors.fill: parent
					clip: true
					spacing: 4

					model: profile.deviceList

					delegate: RowLayout{
						width: parent.width
						Layout.margins : {
							top : 50
						}
						ColumnLayout{
							Text{
								Layout.fillWidth: true
								color: colors.text
								font.bold: true
								Layout.alignment: Qt.AlignLeft
								text: model.deviceId
							}
							Text{
								Layout.fillWidth: true
								color:colors.text
								Layout.alignment: Qt.AlignRight
								text: model.deviceName
							}
						}
						RowLayout{
							Image{
								Layout.preferredWidth: 20
								Layout.preferredHeight: 20
								source: ((model.verificationStatus == VerificationStatus.VERIFIED)?"image://colorimage/:/icons/icons/ui/lock.png?green":
								((model.verificationStatus == VerificationStatus.UNVERIFIED)?"image://colorimage/:/icons/icons/ui/unlock.png?yellow":
								"image://colorimage/:/icons/icons/ui/unlock.png?red"))
							}
							Button{
								id: verifyButton
								text:(model.verificationStatus != VerificationStatus.VERIFIED)?"Verify":"Unverify"
								onClicked: {
									var newFlow = profile.createFlow(false);
									newFlow.userId = profile.userid;
									newFlow.sender = true;
									newFlow.deviceId = model.deviceId;
									if(model.verificationStatus == VerificationStatus.VERIFIED){
										newFlow.unverify();
										deviceVerificationList.updateProfile(newFlow.userId);
									}else{
										deviceVerificationList.add(newFlow.tranId);
										var dialog = deviceVerificationDialog.createObject(userProfileDialog, {flow: newFlow,isRequest:false,tran_id: newFlow.tranId});
										dialog.show();
									}
								}
								Layout.margins:{
									right: 10
								}
								palette {
									button: "white"
								}
								contentItem: Text {
									text: verifyButton.text
									color: "black"
									horizontalAlignment: Text.AlignHCenter
									verticalAlignment: Text.AlignVCenter
								}
							}
						}
					}
				}
			}

			Button{
				id: okbutton
				text:"OK"
				onClicked: userProfileDialog.close()

				Layout.alignment: Qt.AlignRight | Qt.AlignBottom

				Layout.margins : {
					right : 10
					bottom: 5
				}

				palette {
					button: "white"
				}

				contentItem: Text {
					text: okbutton.text
					color: "black"
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
				}
			}
		}

		Item { Layout.fillHeight: true }
	}
}
