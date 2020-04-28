import QtQuick 2.5
import QtQuick.Controls 2.1

Label {
	color: colors.brightText
	horizontalAlignment: Text.AlignHCenter

    leftPadding: 24
    rightPadding: 24
    topPadding: 8
    bottomPadding: 8

	background: Rectangle {
		radius: parent.height / 2
		color: colors.dark
	}
}
