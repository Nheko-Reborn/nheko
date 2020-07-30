#include <algorithm>
#include <cctype>
#include <cstdint>
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

#include "mtx/responses/turn_server.hpp"

Q_DECLARE_METATYPE(std::vector<mtx::events::msg::CallCandidates::Candidate>)
Q_DECLARE_METATYPE(mtx::events::msg::CallCandidates::Candidate)
Q_DECLARE_METATYPE(mtx::responses::TurnServer)

using namespace mtx::events;
using namespace mtx::events::msg;

// https://github.com/vector-im/riot-web/issues/10173
#define STUN_SERVER "stun://turn.matrix.org:3478"

namespace {
std::vector<std::string>
getTurnURIs(const mtx::responses::TurnServer &turnServer);
}

CallManager::CallManager(QSharedPointer<UserSettings> userSettings)
  : QObject(),
    session_(WebRTCSession::instance()),
    turnServerTimer_(this),
    settings_(userSettings)
{
  qRegisterMetaType<std::vector<mtx::events::msg::CallCandidates::Candidate>>();
  qRegisterMetaType<mtx::events::msg::CallCandidates::Candidate>();
  qRegisterMetaType<mtx::responses::TurnServer>();

  connect(&session_, &WebRTCSession::offerCreated, this,
      [this](const std::string &sdp,
             const std::vector<CallCandidates::Candidate> &candidates)
            {
              nhlog::ui()->debug("WebRTC: call id: {} - sending offer", callid_);
              emit newMessage(roomid_, CallInvite{callid_, sdp, 0, timeoutms_});
              emit newMessage(roomid_, CallCandidates{callid_, candidates, 0});

              QTimer::singleShot(timeoutms_, this, [this](){
                  if (session_.state() == WebRTCSession::State::OFFERSENT) {
                      hangUp(CallHangUp::Reason::InviteTimeOut);
                      emit ChatPage::instance()->showNotification("The remote side failed to pick up.");
                  }
              });
            });

  connect(&session_, &WebRTCSession::answerCreated, this,
      [this](const std::string &sdp,
             const std::vector<CallCandidates::Candidate> &candidates)
            {
              nhlog::ui()->debug("WebRTC: call id: {} - sending answer", callid_);
              emit newMessage(roomid_, CallAnswer{callid_, sdp, 0});
              emit newMessage(roomid_, CallCandidates{callid_, candidates, 0});
            });

  connect(&session_, &WebRTCSession::newICECandidate, this,
      [this](const CallCandidates::Candidate &candidate)
            {
              nhlog::ui()->debug("WebRTC: call id: {} - sending ice candidate", callid_);
              emit newMessage(roomid_, CallCandidates{callid_, {candidate}, 0});
            });

  connect(&turnServerTimer_, &QTimer::timeout, this, &CallManager::retrieveTurnServer);

  connect(this, &CallManager::turnServerRetrieved, this,
      [this](const mtx::responses::TurnServer &res)
            {
              nhlog::net()->info("TURN server(s) retrieved from homeserver:");
              nhlog::net()->info("username: {}", res.username);
              nhlog::net()->info("ttl: {} seconds", res.ttl);
              for (const auto &u : res.uris)
                nhlog::net()->info("uri: {}", u);

              // Request new credentials close to expiry
              // See https://tools.ietf.org/html/draft-uberti-behave-turn-rest-00
              turnURIs_ = getTurnURIs(res);
              uint32_t ttl = std::max(res.ttl, UINT32_C(3600));
              if (res.ttl < 3600)
                nhlog::net()->warn("Setting ttl to 1 hour");
              turnServerTimer_.setInterval(ttl * 1000 * 0.9);
      });

  connect(&session_, &WebRTCSession::stateChanged, this,
      [this](WebRTCSession::State state) {
        if (state == WebRTCSession::State::DISCONNECTED) {
          playRingtone("qrc:/media/media/callend.ogg", false);
        }
        else if (state == WebRTCSession::State::ICEFAILED) {
          QString error("Call connection failed.");
          if (turnURIs_.empty())
            error += " Your homeserver has no configured TURN server.";
          emit ChatPage::instance()->showNotification(error);
          hangUp(CallHangUp::Reason::ICEFailed);
        }
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

    auto roomInfo = cache::singleRoomInfo(roomid.toStdString());
    if (roomInfo.member_count != 2) {
      emit ChatPage::instance()->showNotification("Voice calls are limited to 1:1 rooms.");
      return;
    }

    std::string errorMessage;
    if (!session_.init(&errorMessage)) {
      emit ChatPage::instance()->showNotification(QString::fromStdString(errorMessage));
      return;
    }

    roomid_ = roomid;
    session_.setStunServer(settings_->useStunServer() ? STUN_SERVER : "");
    session_.setTurnServers(turnURIs_);

    generateCallID();
    nhlog::ui()->debug("WebRTC: call id: {} - creating invite", callid_);
    std::vector<RoomMember> members(cache::getMembers(roomid.toStdString()));
    const RoomMember &callee = members.front().user_id == utils::localUser() ? members.back() : members.front();
    emit newCallParty(callee.user_id, callee.display_name,
            QString::fromStdString(roomInfo.name), QString::fromStdString(roomInfo.avatar_url));
    playRingtone("qrc:/media/media/ringback.ogg", true);
    if (!session_.createOffer()) {
      emit ChatPage::instance()->showNotification("Problem setting up call.");
      endCall();
    }
}

namespace {
std::string callHangUpReasonString(CallHangUp::Reason reason)
{
  switch (reason) {
    case CallHangUp::Reason::ICEFailed:
      return "ICE failed";
    case CallHangUp::Reason::InviteTimeOut:
      return "Invite time out";
    default:
      return "User";
  }
}
}

void
CallManager::hangUp(CallHangUp::Reason reason)
{
  if (!callid_.empty()) {
    nhlog::ui()->debug("WebRTC: call id: {} - hanging up ({})", callid_,
        callHangUpReasonString(reason));
    emit newMessage(roomid_, CallHangUp{callid_, 0, reason});
    endCall();
  }
}

bool
CallManager::onActiveCall()
{
  return session_.state() != WebRTCSession::State::DISCONNECTED;
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
  const char video[] = "m=video";
  const std::string &sdp = callInviteEvent.content.sdp;
  bool isVideo = std::search(sdp.cbegin(), sdp.cend(), std::cbegin(video), std::cend(video) - 1,
    [](unsigned char c1, unsigned char c2) {return std::tolower(c1) == std::tolower(c2);})
      != sdp.cend();

  nhlog::ui()->debug(std::string("WebRTC: call id: {} - incoming ") + (isVideo ? "video" : "voice") +
      " CallInvite from {}", callInviteEvent.content.call_id, callInviteEvent.sender);

  if (callInviteEvent.content.call_id.empty())
    return;

  if (isVideo) {
    emit newMessage(QString::fromStdString(callInviteEvent.room_id),
        CallHangUp{callInviteEvent.content.call_id, 0, CallHangUp::Reason::InviteTimeOut});
    return;
  }

  auto roomInfo = cache::singleRoomInfo(callInviteEvent.room_id);
  if (onActiveCall() || roomInfo.member_count != 2) {
    emit newMessage(QString::fromStdString(callInviteEvent.room_id),
        CallHangUp{callInviteEvent.content.call_id, 0, CallHangUp::Reason::InviteTimeOut});
    return;
  }

  playRingtone("qrc:/media/media/ring.ogg", true);
  roomid_ = QString::fromStdString(callInviteEvent.room_id);
  callid_ = callInviteEvent.content.call_id;
  remoteICECandidates_.clear();

  std::vector<RoomMember> members(cache::getMembers(callInviteEvent.room_id));
  const RoomMember &caller =
    members.front().user_id == utils::localUser() ? members.back() : members.front();
  emit newCallParty(caller.user_id, caller.display_name,
          QString::fromStdString(roomInfo.name), QString::fromStdString(roomInfo.avatar_url));

  auto dialog = new dialogs::AcceptCall(
      caller.user_id,
      caller.display_name,
      QString::fromStdString(roomInfo.name),
      QString::fromStdString(roomInfo.avatar_url),
      MainWindow::instance());
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

  session_.setStunServer(settings_->useStunServer() ? STUN_SERVER : "");
  session_.setTurnServers(turnURIs_);

  if (!session_.acceptOffer(invite.sdp)) {
    emit ChatPage::instance()->showNotification("Problem setting up call.");
    hangUp();
    return;
  }
  session_.acceptICECandidates(remoteICECandidates_);
  remoteICECandidates_.clear();
}

void
CallManager::handleEvent(const RoomEvent<CallCandidates> &callCandidatesEvent)
{
  if (callCandidatesEvent.sender == utils::localUser().toStdString())
    return;

  nhlog::ui()->debug("WebRTC: call id: {} - incoming CallCandidates from {}",
      callCandidatesEvent.content.call_id, callCandidatesEvent.sender);

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
  nhlog::ui()->debug("WebRTC: call id: {} - incoming CallAnswer from {}",
      callAnswerEvent.content.call_id, callAnswerEvent.sender);

  if (!onActiveCall() && callAnswerEvent.sender == utils::localUser().toStdString() &&
      callid_ == callAnswerEvent.content.call_id) {
    emit ChatPage::instance()->showNotification("Call answered on another device.");
    stopRingtone();
    MainWindow::instance()->hideOverlay();
    return;
  }

  if (onActiveCall() && callid_ == callAnswerEvent.content.call_id) {
    stopRingtone();
    if (!session_.acceptAnswer(callAnswerEvent.content.sdp)) {
      emit ChatPage::instance()->showNotification("Problem setting up call.");
      hangUp();
    }
  }
}

void
CallManager::handleEvent(const RoomEvent<CallHangUp> &callHangUpEvent)
{
  nhlog::ui()->debug("WebRTC: call id: {} - incoming CallHangUp ({}) from {}",
      callHangUpEvent.content.call_id, callHangUpReasonString(callHangUpEvent.content.reason),
         callHangUpEvent.sender);

  if (callid_ == callHangUpEvent.content.call_id) {
    MainWindow::instance()->hideOverlay();
    endCall();
  }
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
CallManager::refreshTurnServer()
{
  turnURIs_.clear();
  turnServerTimer_.start(2000);
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

namespace {
std::vector<std::string>
getTurnURIs(const mtx::responses::TurnServer &turnServer)
{
  // gstreamer expects: turn(s)://username:password@host:port?transport=udp(tcp)
  // where username and password are percent-encoded
  std::vector<std::string> ret;
  for (const auto &uri : turnServer.uris) {
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

      QString encodedUri = QString::fromStdString(scheme) + "://" + 
                           QUrl::toPercentEncoding(QString::fromStdString(turnServer.username)) + ":" +
                           QUrl::toPercentEncoding(QString::fromStdString(turnServer.password)) + "@" +
                           QString::fromStdString(std::string(uri, ++c));
      ret.push_back(encodedUri.toStdString());
    }
  }
  return ret;
}
}

