import QtQuick 2.6
import Qt.labs.qmlmodels 1.0
import com.github.nheko 1.0

import "./delegates"

DelegateChooser {
	role: "type"
	width: chat.width
	roleValue: model.type

	DelegateChoice {
		roleValue: MtxEvent.TextMessage
		TimelineRow { view: chat; TextMessage { id: kid } }
	}
	DelegateChoice {
		roleValue: MtxEvent.NoticeMessage
		TimelineRow { view: chat; NoticeMessage { id: kid } }
	}
	DelegateChoice {
		roleValue: MtxEvent.EmoteMessage
		TimelineRow { view: chat; TextMessage { id: kid } }
	}
	DelegateChoice {
		roleValue: MtxEvent.ImageMessage
		TimelineRow { view: chat; ImageMessage { id: kid } }
	}
	DelegateChoice {
		roleValue: MtxEvent.Sticker
		TimelineRow { view: chat; ImageMessage { id: kid } }
	}
	DelegateChoice {
		roleValue: MtxEvent.FileMessage
		TimelineRow { view: chat; FileMessage { id: kid } }
	}
	DelegateChoice {
		roleValue: MtxEvent.VideoMessage
		TimelineRow { view: chat; PlayableMediaMessage { id: kid } }
	}
	DelegateChoice {
		roleValue: MtxEvent.AudioMessage
		TimelineRow { view: chat; PlayableMediaMessage { id: kid } }
	}
	DelegateChoice {
		roleValue: MtxEvent.Redacted
		TimelineRow { view: chat; Redacted { id: kid } }
	}
	DelegateChoice {
		//roleValue: MtxEvent.Redacted
		TimelineRow { view: chat; Placeholder { id: kid } }
	}
}
