import QtQuick 2.5
import QtQuick.Controls 2.1
import im.nheko 1.0

ImageButton {
    id: indicator

    width: 16
    height: 16
    hoverEnabled: true
    changeColorOnHover: (model.state == MtxEvent.Read)
    cursor: (model.state == MtxEvent.Read) ? Qt.PointingHandCursor : Qt.ArrowCursor
    ToolTip.visible: hovered && model.state != MtxEvent.Empty
    ToolTip.text: {
        switch (model.state) {
        case MtxEvent.Failed:
            return qsTr("Failed");
        case MtxEvent.Sent:
            return qsTr("Sent");
        case MtxEvent.Received:
            return qsTr("Received");
        case MtxEvent.Read:
            return qsTr("Read");
        default:
            return "";
        }
    }
    onClicked: {
        if (model.state == MtxEvent.Read)
            TimelineManager.timeline.readReceiptsAction(model.id);

    }
    image: {
        switch (model.state) {
        case MtxEvent.Failed:
            return ":/icons/icons/ui/remove-symbol.png";
        case MtxEvent.Sent:
            return ":/icons/icons/ui/clock.png";
        case MtxEvent.Received:
            return ":/icons/icons/ui/checkmark.png";
        case MtxEvent.Read:
            return ":/icons/icons/ui/double-tick-indicator.png";
        default:
            return "";
        }
    }
}
