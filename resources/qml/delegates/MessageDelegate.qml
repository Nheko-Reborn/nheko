import QtQuick 2.6
import im.nheko 1.0

DelegateChooser {
	//role: "type" //< not supported in our custom implementation, have to use roleValue
	roleValue: model.type

	DelegateChoice {
		roleValue: MtxEvent.TextMessage
		TextMessage {}
	}
	DelegateChoice {
		roleValue: MtxEvent.NoticeMessage
		NoticeMessage {}
	}
	DelegateChoice {
		roleValue: MtxEvent.EmoteMessage
		TextMessage {}
	}
	DelegateChoice {
		roleValue: MtxEvent.ImageMessage
		ImageMessage {}
	}
	DelegateChoice {
		roleValue: MtxEvent.Sticker
		ImageMessage {}
	}
	DelegateChoice {
		roleValue: MtxEvent.FileMessage
		FileMessage {}
	}
	DelegateChoice {
		roleValue: MtxEvent.VideoMessage
		PlayableMediaMessage {}
	}
	DelegateChoice {
		roleValue: MtxEvent.AudioMessage
		PlayableMediaMessage {}
	}
	DelegateChoice {
		roleValue: MtxEvent.Redacted
		Pill {
			text: qsTr("redacted")
		}
	}
	DelegateChoice {
		roleValue: MtxEvent.Encryption
		Pill {
			text: qsTr("Encryption enabled")
		}
	}
	DelegateChoice {
		roleValue: MtxEvent.Name
		NoticeMessage {
			notice: model.roomName ? qsTr("room name changed to: %1").arg(model.roomName) : qsTr("removed room name")
		}
	}
	DelegateChoice {
		roleValue: MtxEvent.Topic
		NoticeMessage {
			notice: model.roomTopic ? qsTr("topic changed to: %1").arg(model.roomTopic) : qsTr("removed topic")
		}
	}
	DelegateChoice {
		Placeholder {}
	}
}
