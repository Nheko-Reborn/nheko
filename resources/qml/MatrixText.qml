import QtQuick 2.5
import QtQuick.Controls 2.3

TextArea {
	textFormat: TextEdit.RichText
	readOnly: true
	wrapMode: Text.Wrap
	selectByMouse: true
	//color: colors.text
	palette: colors

	padding: 0

	onLinkActivated: {
		if (/^https:\/\/matrix.to\/#\/(@.*)$/.test(link)) chat.model.openUserProfile(/^https:\/\/matrix.to\/#\/(@.*)$/.exec(link)[1])
		else if (/^https:\/\/matrix.to\/#\/(![^\/]*)$/.test(link)) timelineManager.setHistoryView(/^https:\/\/matrix.to\/#\/(!.*)$/.exec(link)[1])
		else if (/^https:\/\/matrix.to\/#\/(![^\/]*)\/(\$.*)$/.test(link)) {
			var match = /^https:\/\/matrix.to\/#\/(![^\/]*)\/(\$.*)$/.exec(link)
			timelineManager.setHistoryView(match[1])
			chat.positionViewAtIndex(chat.model.idToIndex(match[2]), ListView.Contain)
		}
		else Qt.openUrlExternally(link)
	}
	MouseArea
	{
		id: ma
		anchors.fill: parent
		propagateComposedEvents: true
		acceptedButtons: Qt.NoButton
		cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
	}

	ToolTip.visible: hoveredLink
	ToolTip.text: hoveredLink
}
