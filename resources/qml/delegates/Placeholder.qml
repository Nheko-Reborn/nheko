import ".."

MatrixText {
    text: qsTr("unimplemented event: ") + model.data.typeString
    width: parent ? parent.width : undefined
    color: inactiveColors.text
}
