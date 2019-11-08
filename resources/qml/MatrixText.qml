import QtQuick 2.5
import QtQuick.Controls 2.3

TextEdit {
	textFormat: TextEdit.RichText
	readOnly: true
	wrapMode: Text.Wrap
	selectByMouse: true
	color: colors.text

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
		anchors.fill: parent
		onPressed:  mouse.accepted = false
		cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
	}

	ToolTip {
		visible: parent.hoveredLink
		text: parent.hoveredLink
		palette: colors
	}
}
