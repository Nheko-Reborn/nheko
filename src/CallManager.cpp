#include <chrono>

#include <QMediaPlaylist>
#include <QUrl>

#include "CallManager.h"
#include "Cache.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "UserSettingsPage.h"
#include "WebRTCSession.h"

#include "dialogs/AcceptCall.h"

Q_DECLARE_METATYPE(std::vector<mtx::events::msg::CallCandidates::Candidate>)
Q_DECLARE_METATYPE(mtx::responses::TurnServer)

using namespace mtx::events;
using namespace mtx::events::msg;

// TODO Allow altenative in settings
#define STUN_SERVER "stun://turn.matrix.org:3478"

CallManager::CallManager(QSharedPointer<UserSettings> userSettings)
  : QObject(),
    session_(WebRTCSession::instance()),
    turnServerTimer_(this),
    settings_(userSettings)
{
  qRegisterMetaType<std::vector<mtx::events::msg::CallCandidates::Candidate>>();
  qRegisterMetaType<mtx::responses::TurnServer>();

  connect(&session_, &WebRTCSession::offerCreated, this,
      [this](const std::string &sdp,
             const std::vector<mtx::events::msg::CallCandidates::Candidate>& candidates)
            {
              nhlog::ui()->debug("Offer created with callid_ and roomid_: {} {}", callid_, roomid_.toStdString());
              emit newMessage(roomid_, CallInvite{callid_, sdp, 0, timeoutms_});
              emit newMessage(roomid_, CallCandidates{callid_, candidates, 0});
            });

  connect(&session_, &WebRTCSession::answerCreated, this,
      [this](const std::string &sdp,
             const std::vector<mtx::events::msg::CallCandidates::Candidate>& candidates)
            {
              nhlog::ui()->debug("Answer created with callid_ and roomid_: {} {}", callid_, roomid_.toStdString());
              emit newMessage(roomid_, CallAnswer{callid_, sdp, 0});
              emit newMessage(roomid_, CallCandidates{callid_, candidates, 0});
            });

  connect(&turnServerTimer_, &QTimer::timeout, this, &CallManager::retrieveTurnServer);
  turnServerTimer_.start(2000);

  connect(this, &CallManager::turnServerRetrieved, this,
      [this](const mtx::responses::TurnServer &res)
            {
              nhlog::net()->info("TURN server(s) retrieved from homeserver:");
              nhlog::net()->info("username: {}", res.username);
              nhlog::net()->info("ttl: {}", res.ttl);
              for (const auto &u : res.uris)
                nhlog::net()->info("uri: {}", u);

              turnServer_ = res;
              turnServerTimer_.setInterval(res.ttl * 1000 * 0.9);
      });

  connect(&session_, &WebRTCSession::pipelineChanged, this,
      [this](bool started) {
        if (!started)
          playRingtone("qrc:/media/media/callend.ogg", false);
        });

  connect(&player_, &QMediaPlayer::mediaStatusChanged, this,
      [this](QMediaPlayer::MediaStatus status) {
       if (status == QMediaPlayer::LoadedMedia)
         player_.play();
       });
}

void
CallManager::sendInvite(const QString &roomid)
{
    if (onActiveCall())
      return;

    std::vector<RoomMember> members(cache::getMembers(roomid.toStdString()));
    if (members.size() != 2) {
      emit ChatPage::instance()->showNotification("Voice/Video calls are limited to 1:1 rooms");
      return;
    }

    std::string errorMessage;
    if (!session_.init(&errorMessage)) {
      emit ChatPage::instance()->showNotification(QString::fromStdString(errorMessage));
      return;
    }

    roomid_ = roomid;
    setTurnServers();
    session_.setStunServer(settings_->useStunServer() ? STUN_SERVER : "");

    // TODO Add invite timeout
    generateCallID();
    const RoomMember &callee = members.front().user_id == utils::localUser() ? members.back() : members.front();
    emit newCallParty(callee.user_id, callee.display_name);
    playRingtone("qrc:/media/media/ringback.ogg", true);
    if (!session_.createOffer()) {
      emit ChatPage::instance()->showNotification("Problem setting up call");
      endCall();
    }
}

void
CallManager::hangUp()
{
  nhlog::ui()->debug("CallManager::hangUp: roomid_: {}", roomid_.toStdString());
  if (!callid_.empty()) {
    emit newMessage(roomid_, CallHangUp{callid_, 0, CallHangUp::Reason::User});
    endCall();
  }
}

bool
CallManager::onActiveCall()
{
  return session_.isActive();
}

void CallManager::syncEvent(const mtx::events::collections::TimelineEvents &event)
{
  if (handleEvent_<CallInvite>(event) || handleEvent_<CallCandidates>(event)
      || handleEvent_<CallAnswer>(event) || handleEvent_<CallHangUp>(event))
    return;
}

template<typename T>
bool
CallManager::handleEvent_(const mtx::events::collections::TimelineEvents &event)
{
  if (std::holds_alternative<RoomEvent<T>>(event)) {
    handleEvent(std::get<RoomEvent<T>>(event));
    return true;
  }
  return false;
}

void
CallManager::handleEvent(const RoomEvent<CallInvite> &callInviteEvent)
{
  nhlog::ui()->debug("CallManager::incoming CallInvite from {} with id {}", callInviteEvent.sender, callInviteEvent.content.call_id);

  if (callInviteEvent.content.call_id.empty())
    return;

  std::vector<RoomMember> members(cache::getMembers(callInviteEvent.room_id));
  if (onActiveCall() || members.size() != 2) {
    emit newMessage(QString::fromStdString(callInviteEvent.room_id),
        CallHangUp{callInviteEvent.content.call_id, 0, CallHangUp::Reason::InviteTimeOut});
    return;
  }

  playRingtone("qrc:/media/media/ring.ogg", true);
  roomid_ = QString::fromStdString(callInviteEvent.room_id);
  callid_ = callInviteEvent.content.call_id;
  remoteICECandidates_.clear();

  const RoomMember &caller = members.front().user_id == utils::localUser() ? members.back() : members.front();
  emit newCallParty(caller.user_id, caller.display_name);

  auto dialog = new dialogs::AcceptCall(caller.user_id, caller.display_name, MainWindow::instance());
  connect(dialog, &dialogs::AcceptCall::accept, this,
      [this, callInviteEvent](){
        MainWindow::instance()->hideOverlay();
        answerInvite(callInviteEvent.content);});
  connect(dialog, &dialogs::AcceptCall::reject, this,
      [this](){
        MainWindow::instance()->hideOverlay();
        hangUp();});
  MainWindow::instance()->showSolidOverlayModal(dialog);
}

void
CallManager::answerInvite(const CallInvite &invite)
{
  stopRingtone();
  std::string errorMessage;
  if (!session_.init(&errorMessage)) {
    emit ChatPage::instance()->showNotification(QString::fromStdString(errorMessage));
    hangUp();
    return;
  }

  setTurnServers();
  session_.setStunServer(settings_->useStunServer() ? STUN_SERVER : "");

  if (!session_.acceptOffer(invite.sdp)) {
    emit ChatPage::instance()->showNotification("Problem setting up call");
    hangUp();
    return;
  }
  session_.acceptICECandidates(remoteICECandidates_);
  remoteICECandidates_.clear();
}

void
CallManager::handleEvent(const RoomEvent<CallCandidates> &callCandidatesEvent)
{
  nhlog::ui()->debug("CallManager::incoming CallCandidates from {} with id {}", callCandidatesEvent.sender, callCandidatesEvent.content.call_id);
  if (callid_ == callCandidatesEvent.content.call_id) {
    if (onActiveCall())
      session_.acceptICECandidates(callCandidatesEvent.content.candidates);
    else {
      // CallInvite has been received and we're awaiting localUser to accept or reject the call
      for (const auto &c : callCandidatesEvent.content.candidates)
        remoteICECandidates_.push_back(c);
    }
  }
}

void
CallManager::handleEvent(const RoomEvent<CallAnswer> &callAnswerEvent)
{
  nhlog::ui()->debug("CallManager::incoming CallAnswer from {} with id {}", callAnswerEvent.sender, callAnswerEvent.content.call_id);
  if (onActiveCall() && callid_ == callAnswerEvent.content.call_id) {
    stopRingtone();
    if (!session_.acceptAnswer(callAnswerEvent.content.sdp)) {
      emit ChatPage::instance()->showNotification("Problem setting up call");
      hangUp();
    }
  }
}

void
CallManager::handleEvent(const RoomEvent<CallHangUp> &callHangUpEvent)
{
  nhlog::ui()->debug("CallManager::incoming CallHangUp from {} with id {}", callHangUpEvent.sender, callHangUpEvent.content.call_id);
  if (onActiveCall() && callid_ == callHangUpEvent.content.call_id)
    endCall();
}

void
CallManager::generateCallID()
{
  using namespace std::chrono;
  uint64_t ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  callid_ = "c" + std::to_string(ms);
}

void
CallManager::endCall()
{
  stopRingtone();
  session_.end();
  roomid_.clear();
  callid_.clear();
  remoteICECandidates_.clear();
}

void
CallManager::retrieveTurnServer()
{
  http::client()->get_turn_server(
    [this](const mtx::responses::TurnServer &res, mtx::http::RequestErr err) {
        if (err) {
            turnServerTimer_.setInterval(5000);
            return;
        }
        emit turnServerRetrieved(res);
  });
}

void
CallManager::setTurnServers()
{
  // gstreamer expects (percent-encoded): turn(s)://username:password@host:port?transport=udp(tcp)
  std::vector<std::string> uris;
  for (const auto &uri : turnServer_.uris) {
    if (auto c = uri.find(':'); c == std::string::npos) {
      nhlog::ui()->error("Invalid TURN server uri: {}", uri);
      continue;
    }
    else {
      std::string scheme = std::string(uri, 0, c);
      if (scheme != "turn" && scheme != "turns") {
        nhlog::ui()->error("Invalid TURN server uri: {}", uri);
        continue;
      }
      std::string res = scheme + "://" + turnServer_.username + ":" + turnServer_.password
        + "@" + std::string(uri, ++c);
      QString encodedUri = QUrl::toPercentEncoding(QString::fromStdString(res));
      uris.push_back(encodedUri.toStdString());
    }
  }
  if (!uris.empty())
    session_.setTurnServers(uris);
}

void
CallManager::playRingtone(const QString &ringtone, bool repeat)
{
  static QMediaPlaylist playlist;
  playlist.clear();
  playlist.setPlaybackMode(repeat ? QMediaPlaylist::CurrentItemInLoop : QMediaPlaylist::CurrentItemOnce);
  playlist.addMedia(QUrl(ringtone));
  player_.setVolume(100);
  player_.setPlaylist(&playlist);
}

void
CallManager::stopRingtone()
{
  player_.setPlaylist(nullptr);
}
