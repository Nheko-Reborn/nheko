import QtQuick 2.3
import QtQuick.Controls 1.2

Item {
	DeviceVerification {
		id: deviceVerification
	}

	Button {
		text: "Test DeviceVerification"
		onClicked: deviceVerification.show()
	}
}
