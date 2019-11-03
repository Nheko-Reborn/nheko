import ".."

MatrixText {
	text: qsTr("unimplemented event: ") + model.type
	width: parent ? parent.width : undefined
	color: inactiveColors.text
}
