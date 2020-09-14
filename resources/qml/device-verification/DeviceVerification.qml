import QtQuick 2.3
import QtQuick.Controls 2.10
import QtQuick.Window 2.2

import im.nheko 1.0

ApplicationWindow {
	property var flow
	property bool isRequest
	property var tran_id

	title: stack.currentItem.title
	id: dialog

	flags: Qt.Dialog

	palette: colors

	height: stack.implicitHeight
	width: stack.implicitWidth

	Component{
		id: newVerificationRequest
		NewVerificationRequest {}
	}

	Component{
		id: acceptNewVerificationRequest
		AcceptNewVerificationRequest {}
	}

	StackView {
		id: stack
		initialItem: flow.sender == true?newVerificationRequest:acceptNewVerificationRequest
		implicitWidth: currentItem.implicitWidth
		implicitHeight: currentItem.implicitHeight
	}

	Component {
		id: partnerAborted
		PartnerAborted {}
	}

	Component {
		id: timedout
		TimedOut {}
	}

	Component {
		id: verificationSuccess
		VerificationSuccess {}
	}

	Component {
		id: digitVerification
		DigitVerification {}
	}

	Component {
		id: emojiVerification
		EmojiVerification {}
	}

	Connections {
		target: flow
		onVerificationCanceled: stack.replace(partnerAborted)
		onTimedout: stack.replace(timedout)
		onDeviceVerified: stack.replace(verificationSuccess)

		onVerificationRequestAccepted: switch(method) {
			case DeviceVerificationFlow.Decimal: stack.replace(digitVerification); break;
			case DeviceVerificationFlow.Emoji: stack.replace(emojiVerification); break;
		}

		onRefreshProfile: {
			deviceVerificationList.updateProfile(flow.userId);
		}
	}
}
