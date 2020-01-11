import ".."

MatrixText {
	text: qsTr("unimplemented event: ") + model.data.type
	width: parent ? parent.width : undefined
	color: inactiveColors.text
}
