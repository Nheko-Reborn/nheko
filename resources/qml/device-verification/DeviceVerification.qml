import QtQuick 2.3
import QtQuick.Controls 2.10
import QtQuick.Window 2.2
import QtQuick.Layouts 1.10
import Qt.labs.settings 1.0

import im.nheko 1.0

ApplicationWindow {
    property bool sender: true
	title: stack.currentItem.title
	id: dialog

	flags: Qt.Dialog

	palette: colors

	Settings {
		id: settings
		category: "user"
		property bool emoji_font_family: true
	}

	height: stack.implicitHeight
	width: stack.implicitWidth
	StackView {
		id: stack
		initialItem: sender == true?newVerificationRequest:acceptNewVerificationRequest
		implicitWidth: currentItem.implicitWidth
		implicitHeight: currentItem.implicitHeight
	}

	onClosing: stack.replace(newVerificationRequest)

	property var flow
	Connections {
		target: flow
		onVerificationCanceled: stack.replace(partnerAborted)
		onTimedout: stack.replace(timedout)
		onDeviceVerified: stack.replace(verificationSuccess)

		onVerificationRequestAccepted: switch(method) {
			case DeviceVerificationFlow.Decimal: stack.replace(digitVerification); break;
			case DeviceVerificationFlow.Emoji: stack.replace(emojiVerification); break;
		}
	}

	Component {
		id: newVerificationRequest
		Pane {
			property string title: "Sending Device Verification Request"
			ColumnLayout {
				spacing: 16
				Label {
					Layout.maximumWidth: 400
					Layout.fillHeight: true
					Layout.fillWidth: true
					wrapMode: Text.Wrap
					text: "A new device was added."

					verticalAlignment: Text.AlignVCenter
				}

				Label {
					Layout.maximumWidth: 400
					Layout.fillHeight: true
					Layout.fillWidth: true
					wrapMode: Text.Wrap
					text: "The device may have been added by you signing in from another client or physical device. To ensure that no malicious user can eavesdrop on your encrypted communications, you should verify the new device."

					verticalAlignment: Text.AlignVCenter
				}

				RowLayout {
					Button {
						Layout.alignment: Qt.AlignLeft
						text: "Cancel"
						onClicked: { 
							dialog.close(); 
							flow.cancelVerification();
							// deviceVerificationList.remove(flow.tranId);
							delete flow; 
						}
					}
					Item {
						Layout.fillWidth: true
					}
					Button {
						Layout.alignment: Qt.AlignRight
						text: "Start verification"
						onClicked: { stack.replace(awaitingVerificationRequestAccept); flow.startVerificationRequest(); }
					}
				}
			}
		}
	}

	Component {
		id: acceptNewVerificationRequest
		Pane {
			property string title: "Recieving Device Verification Request"
			ColumnLayout {
				spacing: 16

				Label {
					Layout.maximumWidth: 400
					Layout.fillHeight: true
					Layout.fillWidth: true
					wrapMode: Text.Wrap
					text: "The device was requested to be verified"

					verticalAlignment: Text.AlignVCenter
				}

				RowLayout {
					RadioButton {
						Layout.alignment: Qt.AlignLeft
						text: "Decimal"
						onClicked: { flow.method = DeviceVerificationFlow.Decimal }
					}
					Item {
						Layout.fillWidth: true
					}
					RadioButton {
						Layout.alignment: Qt.AlignRight
						text: "Emoji"
						onClicked: { flow.method = DeviceVerificationFlow.Emoji }
					}
				}

				RowLayout {
					Button {
						Layout.alignment: Qt.AlignLeft
						text: "Deny"
						onClicked: { 
							dialog.close(); 
							flow.cancelVerification();
							// deviceVerificationList.remove(flow.tranId);
							delete flow; 
						}
					}
					Item {
						Layout.fillWidth: true
					}
					Button {
						Layout.alignment: Qt.AlignRight
						text: "Accept"
						onClicked: { stack.replace(awaitingVerificationRequestAccept); flow.acceptVerificationRequest(); }
					}
				}
			}
		}
	}

	Component {
		id: awaitingVerificationRequestAccept
		Pane {
			property string title: "Waiting for other party"
			ColumnLayout {
				spacing: 16
				Label {
					Layout.maximumWidth: 400
					Layout.fillHeight: true
					Layout.fillWidth: true
					wrapMode: Text.Wrap
					id: content
					text: "Waiting for other side to accept the verification request."

					verticalAlignment: Text.AlignVCenter
				}

				BusyIndicator {
					Layout.alignment: Qt.AlignHCenter
				}
				RowLayout {
					Button {
						Layout.alignment: Qt.AlignLeft
						text: "Cancel"
						onClicked: { 
							dialog.close(); 
							flow.cancelVerification();
							// deviceVerificationList.remove(flow.tranId);
							delete flow; 
						}
					}
					Item {
						Layout.fillWidth: true
					}
				}
			}
		}
	}

	Component {
		id: digitVerification
		Pane {
			property string title: "Verification Code"
			ColumnLayout {
				spacing: 16
				Label {
					Layout.maximumWidth: 400
					Layout.fillHeight: true
					Layout.fillWidth: true
					wrapMode: Text.Wrap
					text: "Please verify the following digits. You should see the same numbers on both sides. If they differ, please press 'They do not match!' to abort verification!"

					verticalAlignment: Text.AlignVCenter
				}

				RowLayout {
					Layout.alignment: Qt.AlignHCenter
					Label {
						font.pixelSize: Qt.application.font.pixelSize * 2
						text: flow.sasList[0]
					}
					Label {
						font.pixelSize: Qt.application.font.pixelSize * 2
						text: flow.sasList[1]
					}
					Label {
						font.pixelSize: Qt.application.font.pixelSize * 2
						text: flow.sasList[2]
					}
				}

				RowLayout {
					Button {
						Layout.alignment: Qt.AlignLeft
						text: "They do not match!"
						onClicked: { 
							dialog.close(); 
							flow.cancelVerification();
							// deviceVerificationList.remove(flow.tranId);
							delete flow; 
						}
					}
					Item {
						Layout.fillWidth: true
					}
					Button {
						Layout.alignment: Qt.AlignRight
						text: "They match."
						onClicked: { stack.replace(awaitingVerificationConfirmation); flow.acceptDevice(); }
					}
				}
			}
		}
	}

	Component {
		id: emojiVerification
		Pane {
			property string title: "Verification Code"
			ColumnLayout {
				spacing: 16
				Label {
					Layout.maximumWidth: 400
					Layout.fillHeight: true
					Layout.fillWidth: true
					wrapMode: Text.Wrap
					text: "Please verify the following emoji. You should see the same emoji on both sides. If they differ, please press 'They do not match!' to abort verification!"

					verticalAlignment: Text.AlignVCenter
				}

				RowLayout {
					Layout.alignment: Qt.AlignHCenter

					id: emojis

					property var mapping: [
						{"number": 0, "emoji": "🐶", "description": "Dog", "unicode": "U+1F436"},
						{"number": 1, "emoji": "🐱", "description": "Cat", "unicode": "U+1F431"},
						{"number": 2, "emoji": "🦁", "description": "Lion", "unicode": "U+1F981"},
						{"number": 3, "emoji": "🐎", "description": "Horse", "unicode": "U+1F40E"},
						{"number": 4, "emoji": "🦄", "description": "Unicorn", "unicode": "U+1F984"},
						{"number": 5, "emoji": "🐷", "description": "Pig", "unicode": "U+1F437"},
						{"number": 6, "emoji": "🐘", "description": "Elephant", "unicode": "U+1F418"},
						{"number": 7, "emoji": "🐰", "description": "Rabbit", "unicode": "U+1F430"},
						{"number": 8, "emoji": "🐼", "description": "Panda", "unicode": "U+1F43C"},
						{"number": 9, "emoji": "🐓", "description": "Rooster", "unicode": "U+1F413"},
						{"number": 10, "emoji": "🐧", "description": "Penguin", "unicode": "U+1F427"},
						{"number": 11, "emoji": "🐢", "description": "Turtle", "unicode": "U+1F422"},
						{"number": 12, "emoji": "🐟", "description": "Fish", "unicode": "U+1F41F"},
						{"number": 13, "emoji": "🐙", "description": "Octopus", "unicode": "U+1F419"},
						{"number": 14, "emoji": "🦋", "description": "Butterfly", "unicode": "U+1F98B"},
						{"number": 15, "emoji": "🌷", "description": "Flower", "unicode": "U+1F337"},
						{"number": 16, "emoji": "🌳", "description": "Tree", "unicode": "U+1F333"},
						{"number": 17, "emoji": "🌵", "description": "Cactus", "unicode": "U+1F335"},
						{"number": 18, "emoji": "🍄", "description": "Mushroom", "unicode": "U+1F344"},
						{"number": 19, "emoji": "🌏", "description": "Globe", "unicode": "U+1F30F"},
						{"number": 20, "emoji": "🌙", "description": "Moon", "unicode": "U+1F319"},
						{"number": 21, "emoji": "☁️", "description": "Cloud", "unicode": "U+2601U+FE0F"},
						{"number": 22, "emoji": "🔥", "description": "Fire", "unicode": "U+1F525"},
						{"number": 23, "emoji": "🍌", "description": "Banana", "unicode": "U+1F34C"},
						{"number": 24, "emoji": "🍎", "description": "Apple", "unicode": "U+1F34E"},
						{"number": 25, "emoji": "🍓", "description": "Strawberry", "unicode": "U+1F353"},
						{"number": 26, "emoji": "🌽", "description": "Corn", "unicode": "U+1F33D"},
						{"number": 27, "emoji": "🍕", "description": "Pizza", "unicode": "U+1F355"},
						{"number": 28, "emoji": "🎂", "description": "Cake", "unicode": "U+1F382"},
						{"number": 29, "emoji": "❤️", "description": "Heart", "unicode": "U+2764U+FE0F"},
						{"number": 30, "emoji": "😀", "description": "Smiley", "unicode": "U+1F600"},
						{"number": 31, "emoji": "🤖", "description": "Robot", "unicode": "U+1F916"},
						{"number": 32, "emoji": "🎩", "description": "Hat", "unicode": "U+1F3A9"},
						{"number": 33, "emoji": "👓", "description": "Glasses", "unicode": "U+1F453"},
						{"number": 34, "emoji": "🔧", "description": "Spanner", "unicode": "U+1F527"},
						{"number": 35, "emoji": "🎅", "description": "Santa", "unicode": "U+1F385"},
						{"number": 36, "emoji": "👍", "description": "Thumbs Up", "unicode": "U+1F44D"},
						{"number": 37, "emoji": "☂️", "description": "Umbrella", "unicode": "U+2602U+FE0F"},
						{"number": 38, "emoji": "⌛", "description": "Hourglass", "unicode": "U+231B"},
						{"number": 39, "emoji": "⏰", "description": "Clock", "unicode": "U+23F0"},
						{"number": 40, "emoji": "🎁", "description": "Gift", "unicode": "U+1F381"},
						{"number": 41, "emoji": "💡", "description": "Light Bulb", "unicode": "U+1F4A1"},
						{"number": 42, "emoji": "📕", "description": "Book", "unicode": "U+1F4D5"},
						{"number": 43, "emoji": "✏️", "description": "Pencil", "unicode": "U+270FU+FE0F"},
						{"number": 44, "emoji": "📎", "description": "Paperclip", "unicode": "U+1F4CE"},
						{"number": 45, "emoji": "✂️", "description": "Scissors", "unicode": "U+2702U+FE0F"},
						{"number": 46, "emoji": "🔒", "description": "Lock", "unicode": "U+1F512"},
						{"number": 47, "emoji": "🔑", "description": "Key", "unicode": "U+1F511"},
						{"number": 48, "emoji": "🔨", "description": "Hammer", "unicode": "U+1F528"},
						{"number": 49, "emoji": "☎️", "description": "Telephone", "unicode": "U+260EU+FE0F"},
						{"number": 50, "emoji": "🏁", "description": "Flag", "unicode": "U+1F3C1"},
						{"number": 51, "emoji": "🚂", "description": "Train", "unicode": "U+1F682"},
						{"number": 52, "emoji": "🚲", "description": "Bicycle", "unicode": "U+1F6B2"},
						{"number": 53, "emoji": "✈️", "description": "Aeroplane", "unicode": "U+2708U+FE0F"},
						{"number": 54, "emoji": "🚀", "description": "Rocket", "unicode": "U+1F680"},
						{"number": 55, "emoji": "🏆", "description": "Trophy", "unicode": "U+1F3C6"},
						{"number": 56, "emoji": "⚽", "description": "Ball", "unicode": "U+26BD"},
						{"number": 57, "emoji": "🎸", "description": "Guitar", "unicode": "U+1F3B8"},
						{"number": 58, "emoji": "🎺", "description": "Trumpet", "unicode": "U+1F3BA"},
						{"number": 59, "emoji": "🔔", "description": "Bell", "unicode": "U+1F514"},
						{"number": 60, "emoji": "⚓", "description": "Anchor", "unicode": "U+2693"},
						{"number": 61, "emoji": "🎧", "description": "Headphones", "unicode": "U+1F3A7"},
						{"number": 62, "emoji": "📁", "description": "Folder", "unicode": "U+1F4C1"},
						{"number": 63, "emoji": "📌", "description": "Pin", "unicode": "U+1F4CC"}
					]

					Repeater {
						id: repeater
						model: 7
						delegate: Rectangle {
							color: "transparent"
							implicitHeight: Qt.application.font.pixelSize * 8
							implicitWidth: col.width
							ColumnLayout {
								id: col
								anchors.bottom: parent.bottom
								property var emoji: emojis.mapping[flow.sasList[index]]
								Label {
									//height: font.pixelSize * 2
									Layout.alignment: Qt.AlignHCenter
									text: col.emoji.emoji
									font.pixelSize: Qt.application.font.pixelSize * 2
									font.family: settings.emoji_font_family
								}
								Label {
									Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
									text: col.emoji.description
								}
							}
						}
					}
				}

				RowLayout {
					Button {
						Layout.alignment: Qt.AlignLeft
						text: "They do not match!"
						onClicked: { 
							dialog.close(); 
							flow.cancelVerification();
							// deviceVerificationList.remove(flow.tranId);
							delete flow; 
						}
					}
					Item {
						Layout.fillWidth: true
					}
					Button {
						Layout.alignment: Qt.AlignRight
						text: "They match."
						onClicked: { stack.replace(awaitingVerificationConfirmation); flow.acceptDevice(); }
					}
				}
			}
		}
	}

	Component {
		id: awaitingVerificationConfirmation
		Pane {
			property string title: "Awaiting Confirmation"
			ColumnLayout {
				spacing: 16
				Label {
					Layout.maximumWidth: 400
					Layout.fillHeight: true
					Layout.fillWidth: true
					wrapMode: Text.Wrap
					id: content
					text: "Waiting for other side to complete verification."

					verticalAlignment: Text.AlignVCenter
				}

				BusyIndicator {
					Layout.alignment: Qt.AlignHCenter
				}
				RowLayout {
					Button {
						Layout.alignment: Qt.AlignLeft
						text: "Cancel"
						onClicked: { 
							dialog.close(); 
							flow.cancelVerification(); 
							// deviceVerificationList.remove(flow.tranId);
							delete flow;
						}
					}
					Item {
						Layout.fillWidth: true
					}
				}
			}
		}
	}

	Component {
		id: verificationSuccess
		Pane {
			property string title: "Successful Verification"
			ColumnLayout {
				spacing: 16
				Label {
					Layout.maximumWidth: 400
					Layout.fillHeight: true
					Layout.fillWidth: true
					wrapMode: Text.Wrap
					id: content
					text: "Verification successful! Both sides verified their devices!"

					verticalAlignment: Text.AlignVCenter
				}

				RowLayout {
					Item {
						Layout.fillWidth: true
					}
					Button {
						Layout.alignment: Qt.AlignRight
						text: "Close"
						onClicked: {
							dialog.close()
							// deviceVerificationList.remove(flow.tranId);
							delete flow;
						}
					}
				}
			}
		}
	}

	Component {
		id: partnerAborted
		Pane {
			property string title: "Verification aborted!"
			ColumnLayout {
				spacing: 16
				Label {
					Layout.maximumWidth: 400
					Layout.fillHeight: true
					Layout.fillWidth: true
					wrapMode: Text.Wrap
					id: content
					text: "Verification canceled by the other party!"

					verticalAlignment: Text.AlignVCenter
				}

				RowLayout {
					Item {
						Layout.fillWidth: true
					}
					Button {
						Layout.alignment: Qt.AlignRight
						text: "Close"
						onClicked: {
							dialog.close()
							// deviceVerificationList.remove(flow.tranId);
							delete flow;
						}
					}
				}
			}
		}
	}

	Component {
		id: timedout
		Pane {
			property string title: "Verification timed out"
			ColumnLayout {
				spacing: 16
				Text {
					Layout.maximumWidth: 400
					Layout.fillHeight: true
					Layout.fillWidth: true
					wrapMode: Text.Wrap
					id: content
					text: "Device verification timed out."

					verticalAlignment: Text.AlignVCenter
				}

				RowLayout {
					Item {
						Layout.fillWidth: true
					}
					Button {
						Layout.alignment: Qt.AlignRight
						text: "Close"
						onClicked: {
							dialog.close()
							// deviceVerificationList.remove(flow.tranId);
							delete flow;
						}
					}
				}
			}
		}
	}
}
