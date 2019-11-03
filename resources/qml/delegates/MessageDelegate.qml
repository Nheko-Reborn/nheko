import QtQuick 2.6
import com.github.nheko 1.0

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
		Redacted {}
	}
	DelegateChoice {
		Placeholder {}
	}
}
