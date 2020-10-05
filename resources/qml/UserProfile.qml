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
	minimumHeight: 420

	palette: colors

	Component {
		id: deviceVerificationDialog
		DeviceVerification {}
	}

	ColumnLayout{
		id: contentL

		anchors.fill: parent
		anchors.margins: 10

		spacing: 10

		Avatar {
			url: profile.avatarUrl.replace("mxc://", "image://MxcImage/")
			height: 130
			width: 130
			displayName: profile.displayName
			userid: profile.userid
			Layout.alignment: Qt.AlignHCenter
		}

		Label {
			text: profile.displayName
			fontSizeMode: Text.HorizontalFit
			font.pixelSize: 20
			color: TimelineManager.userColor(profile.userid, colors.window)
			font.bold: true
			Layout.alignment: Qt.AlignHCenter
		}

		MatrixText {
			text: profile.userid
			font.pixelSize: 15
			Layout.alignment: Qt.AlignHCenter
		}

		Button {
			id: verifyUserButton
			text: "Verify"
			Layout.alignment: Qt.AlignHCenter
			enabled: !profile.isUserVerified
			visible: !profile.isUserVerified

			onClicked: profile.verify()
		}

		RowLayout {
			Layout.alignment: Qt.AlignHCenter
			spacing: 8

			ImageButton {
				image:":/icons/icons/ui/do-not-disturb-rounded-sign.png"
				hoverEnabled: true
				ToolTip.visible: hovered
				ToolTip.text: qsTr("Ban the user")
				onClicked: profile.banUser()
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
				hoverEnabled: true
				ToolTip.visible: hovered
				ToolTip.text: qsTr("Start a private chat")
				onClicked: profile.startChat()
			}
			ImageButton{
				image:":/icons/icons/ui/round-remove-button.png"
				hoverEnabled: true
				ToolTip.visible: hovered
				ToolTip.text: qsTr("Kick the user")
				onClicked: profile.kickUser()
			}
		}

		ListView{
			id: devicelist

			Layout.fillHeight: true
			Layout.minimumHeight: 200
			Layout.fillWidth: true

			clip: true
			spacing: 8
			boundsBehavior: Flickable.StopAtBounds

			model: profile.deviceList

			delegate: RowLayout{
				width: devicelist.width
				spacing: 4

				ColumnLayout{
					spacing: 0
					Text{
						Layout.fillWidth: true
						Layout.alignment: Qt.AlignLeft

						elide: Text.ElideRight
						font.bold: true
						color: colors.text
						text: model.deviceId
					}
					Text{
						Layout.fillWidth: true
						Layout.alignment: Qt.AlignRight

						elide: Text.ElideRight
						color: colors.text
						text: model.deviceName
					}
				}

				Image{
					Layout.preferredHeight: 16
					Layout.preferredWidth: 16

					source: ((model.verificationStatus == VerificationStatus.VERIFIED)?"image://colorimage/:/icons/icons/ui/lock.png?green":
					((model.verificationStatus == VerificationStatus.UNVERIFIED)?"image://colorimage/:/icons/icons/ui/unlock.png?yellow":
					"image://colorimage/:/icons/icons/ui/unlock.png?red"))
				}
				Button{
					id: verifyButton
					text: (model.verificationStatus != VerificationStatus.VERIFIED)?"Verify":"Unverify"
					onClicked: {
						if(model.verificationStatus == VerificationStatus.VERIFIED){
							profile.unverify(model.deviceId)
							deviceVerificationList.updateProfile(newFlow.userId);
						}else{
							profile.verify(model.deviceId);
						}
					}
				}
			}
		}
	}

	footer: DialogButtonBox {
		standardButtons: DialogButtonBox.Ok

		onAccepted: userProfileDialog.close()
	}
}
