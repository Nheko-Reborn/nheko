#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>

#include <QMediaPlaylist>
#include <QUrl>

#include "Cache.h"
#include "CallDevices.h"
#include "CallManager.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "UserSettingsPage.h"
#include "Utils.h"

#include "mtx/responses/turn_server.hpp"

Q_DECLARE_METATYPE(std::vector<mtx::events::msg::CallCandidates::Candidate>)
Q_DECLARE_METATYPE(mtx::events::msg::CallCandidates::Candidate)
Q_DECLARE_METATYPE(mtx::responses::TurnServer)

using namespace mtx::events;
using namespace mtx::events::msg;

using webrtc::CallType;

namespace {
std::vector<std::string>
getTurnURIs(const mtx::responses::TurnServer &turnServer);
}

CallManager::CallManager(QObject *parent)
  : QObject(parent)
  , session_(WebRTCSession::instance())
  , turnServerTimer_(this)
{
        qRegisterMetaType<std::vector<mtx::events::msg::CallCandidates::Candidate>>();
        qRegisterMetaType<mtx::events::msg::CallCandidates::Candidate>();
        qRegisterMetaType<mtx::responses::TurnServer>();

        connect(
          &session_,
          &WebRTCSession::offerCreated,
          this,
          [this](const std::string &sdp, const std::vector<CallCandidates::Candidate> &candidates) {
                  nhlog::ui()->debug("WebRTC: call id: {} - sending offer", callid_);
                  emit newMessage(roomid_, CallInvite{callid_, sdp, 0, timeoutms_});
                  emit newMessage(roomid_, CallCandidates{callid_, candidates, 0});
                  std::string callid(callid_);
                  QTimer::singleShot(timeoutms_, this, [this, callid]() {
                          if (session_.state() == webrtc::State::OFFERSENT && callid == callid_) {
                                  hangUp(CallHangUp::Reason::InviteTimeOut);
                                  emit ChatPage::instance()->showNotification(
                                    "The remote side failed to pick up.");
                          }
                  });
          });

        connect(
          &session_,
          &WebRTCSession::answerCreated,
          this,
          [this](const std::string &sdp, const std::vector<CallCandidates::Candidate> &candidates) {
                  nhlog::ui()->debug("WebRTC: call id: {} - sending answer", callid_);
                  emit newMessage(roomid_, CallAnswer{callid_, sdp, 0});
                  emit newMessage(roomid_, CallCandidates{callid_, candidates, 0});
          });

        connect(&session_,
                &WebRTCSession::newICECandidate,
                this,
                [this](const CallCandidates::Candidate &candidate) {
                        nhlog::ui()->debug("WebRTC: call id: {} - sending ice candidate", callid_);
                        emit newMessage(roomid_, CallCandidates{callid_, {candidate}, 0});
                });

        connect(&turnServerTimer_, &QTimer::timeout, this, &CallManager::retrieveTurnServer);

        connect(this,
                &CallManager::turnServerRetrieved,
                this,
                [this](const mtx::responses::TurnServer &res) {
                        nhlog::net()->info("TURN server(s) retrieved from homeserver:");
                        nhlog::net()->info("username: {}", res.username);
                        nhlog::net()->info("ttl: {} seconds", res.ttl);
                        for (const auto &u : res.uris)
                                nhlog::net()->info("uri: {}", u);

                        // Request new credentials close to expiry
                        // See https://tools.ietf.org/html/draft-uberti-behave-turn-rest-00
                        turnURIs_    = getTurnURIs(res);
                        uint32_t ttl = std::max(res.ttl, UINT32_C(3600));
                        if (res.ttl < 3600)
                                nhlog::net()->warn("Setting ttl to 1 hour");
                        turnServerTimer_.setInterval(ttl * 1000 * 0.9);
                });

        connect(&session_, &WebRTCSession::stateChanged, this, [this](webrtc::State state) {
                switch (state) {
                case webrtc::State::DISCONNECTED:
                        playRingtone(QUrl("qrc:/media/media/callend.ogg"), false);
                        clear();
                        break;
                case webrtc::State::ICEFAILED: {
                        QString error("Call connection failed.");
                        if (turnURIs_.empty())
                                error += " Your homeserver has no configured TURN server.";
                        emit ChatPage::instance()->showNotification(error);
                        hangUp(CallHangUp::Reason::ICEFailed);
                        break;
                }
                default:
                        break;
                }
                emit newCallState();
        });

        connect(&CallDevices::instance(),
                &CallDevices::devicesChanged,
                this,
                &CallManager::devicesChanged);

        connect(&player_,
                &QMediaPlayer::mediaStatusChanged,
                this,
                [this](QMediaPlayer::MediaStatus status) {
                        if (status == QMediaPlayer::LoadedMedia)
                                player_.play();
                });

        connect(&player_,
                QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
                [this](QMediaPlayer::Error error) {
                        stopRingtone();
                        switch (error) {
                        case QMediaPlayer::FormatError:
                        case QMediaPlayer::ResourceError:
                                nhlog::ui()->error("WebRTC: valid ringtone file not found");
                                break;
                        case QMediaPlayer::AccessDeniedError:
                                nhlog::ui()->error("WebRTC: access to ringtone file denied");
                                break;
                        default:
                                nhlog::ui()->error("WebRTC: unable to play ringtone");
                                break;
                        }
                });
}

void
CallManager::sendInvite(const QString &roomid, CallType callType)
{
        if (isOnCall())
                return;
        if (callType == CallType::SCREEN && !screenShareSupported())
                return;

        auto roomInfo = cache::singleRoomInfo(roomid.toStdString());
        if (roomInfo.member_count != 2) {
                emit ChatPage::instance()->showNotification("Calls are limited to 1:1 rooms.");
                return;
        }

        std::string errorMessage;
        if (!session_.havePlugins(false, &errorMessage) ||
            ((callType == CallType::VIDEO || callType == CallType::SCREEN) &&
             !session_.havePlugins(true, &errorMessage))) {
                emit ChatPage::instance()->showNotification(QString::fromStdString(errorMessage));
                return;
        }

        callType_ = callType;
        roomid_   = roomid;
        session_.setTurnServers(turnURIs_);
        generateCallID();
        std::string strCallType = callType_ == CallType::VOICE
                                    ? "voice"
                                    : (callType_ == CallType::VIDEO ? "video" : "screen");
        nhlog::ui()->debug("WebRTC: call id: {} - creating {} invite", callid_, strCallType);
        std::vector<RoomMember> members(cache::getMembers(roomid.toStdString()));
        const RoomMember &callee =
          members.front().user_id == utils::localUser() ? members.back() : members.front();
        callParty_          = callee.display_name.isEmpty() ? callee.user_id : callee.display_name;
        callPartyAvatarUrl_ = QString::fromStdString(roomInfo.avatar_url);
        emit newInviteState();
        playRingtone(QUrl("qrc:/media/media/ringback.ogg"), true);
        if (!session_.createOffer(callType)) {
                emit ChatPage::instance()->showNotification("Problem setting up call.");
                endCall();
        }
}

namespace {
std::string
callHangUpReasonString(CallHangUp::Reason reason)
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
                nhlog::ui()->debug(
                  "WebRTC: call id: {} - hanging up ({})", callid_, callHangUpReasonString(reason));
                emit newMessage(roomid_, CallHangUp{callid_, 0, reason});
                endCall();
        }
}

void
CallManager::syncEvent(const mtx::events::collections::TimelineEvents &event)
{
#ifdef GSTREAMER_AVAILABLE
        if (handleEvent_<CallInvite>(event) || handleEvent_<CallCandidates>(event) ||
            handleEvent_<CallAnswer>(event) || handleEvent_<CallHangUp>(event))
                return;
#else
        (void)event;
#endif
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
        const char video[]     = "m=video";
        const std::string &sdp = callInviteEvent.content.sdp;
        bool isVideo           = std::search(sdp.cbegin(),
                                   sdp.cend(),
                                   std::cbegin(video),
                                   std::cend(video) - 1,
                                   [](unsigned char c1, unsigned char c2) {
                                           return std::tolower(c1) == std::tolower(c2);
                                   }) != sdp.cend();

        nhlog::ui()->debug("WebRTC: call id: {} - incoming {} CallInvite from {}",
                           callInviteEvent.content.call_id,
                           (isVideo ? "video" : "voice"),
                           callInviteEvent.sender);

        if (callInviteEvent.content.call_id.empty())
                return;

        auto roomInfo = cache::singleRoomInfo(callInviteEvent.room_id);
        if (isOnCall() || roomInfo.member_count != 2) {
                emit newMessage(QString::fromStdString(callInviteEvent.room_id),
                                CallHangUp{callInviteEvent.content.call_id,
                                           0,
                                           CallHangUp::Reason::InviteTimeOut});
                return;
        }

        const QString &ringtone = ChatPage::instance()->userSettings()->ringtone();
        if (ringtone != "Mute")
                playRingtone(ringtone == "Default" ? QUrl("qrc:/media/media/ring.ogg")
                                                   : QUrl::fromLocalFile(ringtone),
                             true);
        roomid_ = QString::fromStdString(callInviteEvent.room_id);
        callid_ = callInviteEvent.content.call_id;
        remoteICECandidates_.clear();

        std::vector<RoomMember> members(cache::getMembers(callInviteEvent.room_id));
        const RoomMember &caller =
          members.front().user_id == utils::localUser() ? members.back() : members.front();
        callParty_          = caller.display_name.isEmpty() ? caller.user_id : caller.display_name;
        callPartyAvatarUrl_ = QString::fromStdString(roomInfo.avatar_url);

        haveCallInvite_ = true;
        callType_       = isVideo ? CallType::VIDEO : CallType::VOICE;
        inviteSDP_      = callInviteEvent.content.sdp;
        emit newInviteState();
}

void
CallManager::acceptInvite()
{
        if (!haveCallInvite_)
                return;

        stopRingtone();
        std::string errorMessage;
        if (!session_.havePlugins(false, &errorMessage) ||
            (callType_ == CallType::VIDEO && !session_.havePlugins(true, &errorMessage))) {
                emit ChatPage::instance()->showNotification(QString::fromStdString(errorMessage));
                hangUp();
                return;
        }

        session_.setTurnServers(turnURIs_);
        if (!session_.acceptOffer(inviteSDP_)) {
                emit ChatPage::instance()->showNotification("Problem setting up call.");
                hangUp();
                return;
        }
        session_.acceptICECandidates(remoteICECandidates_);
        remoteICECandidates_.clear();
        haveCallInvite_ = false;
        emit newInviteState();
}

void
CallManager::handleEvent(const RoomEvent<CallCandidates> &callCandidatesEvent)
{
        if (callCandidatesEvent.sender == utils::localUser().toStdString())
                return;

        nhlog::ui()->debug("WebRTC: call id: {} - incoming CallCandidates from {}",
                           callCandidatesEvent.content.call_id,
                           callCandidatesEvent.sender);

        if (callid_ == callCandidatesEvent.content.call_id) {
                if (isOnCall())
                        session_.acceptICECandidates(callCandidatesEvent.content.candidates);
                else {
                        // CallInvite has been received and we're awaiting localUser to accept or
                        // reject the call
                        for (const auto &c : callCandidatesEvent.content.candidates)
                                remoteICECandidates_.push_back(c);
                }
        }
}

void
CallManager::handleEvent(const RoomEvent<CallAnswer> &callAnswerEvent)
{
        nhlog::ui()->debug("WebRTC: call id: {} - incoming CallAnswer from {}",
                           callAnswerEvent.content.call_id,
                           callAnswerEvent.sender);

        if (callAnswerEvent.sender == utils::localUser().toStdString() &&
            callid_ == callAnswerEvent.content.call_id) {
                if (!isOnCall()) {
                        emit ChatPage::instance()->showNotification(
                          "Call answered on another device.");
                        stopRingtone();
                        haveCallInvite_ = false;
                        emit newInviteState();
                }
                return;
        }

        if (isOnCall() && callid_ == callAnswerEvent.content.call_id) {
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
                           callHangUpEvent.content.call_id,
                           callHangUpReasonString(callHangUpEvent.content.reason),
                           callHangUpEvent.sender);

        if (callid_ == callHangUpEvent.content.call_id)
                endCall();
}

void
CallManager::toggleMicMute()
{
        session_.toggleMicMute();
        emit micMuteChanged();
}

bool
CallManager::callsSupported()
{
#ifdef GSTREAMER_AVAILABLE
        return true;
#else
        return false;
#endif
}

bool
CallManager::screenShareSupported()
{
        return std::getenv("DISPLAY") != nullptr;
}

bool
CallManager::haveVideo() const
{
        return callType() == CallType::VIDEO ||
               (callType() == CallType::SCREEN &&
                (ChatPage::instance()->userSettings()->screenShareRemoteVideo() &&
                 !session_.isRemoteVideoRecvOnly()));
}

QStringList
CallManager::devices(bool isVideo) const
{
        QStringList ret;
        const QString &defaultDevice = isVideo ? ChatPage::instance()->userSettings()->camera()
                                               : ChatPage::instance()->userSettings()->microphone();
        std::vector<std::string> devices =
          CallDevices::instance().names(isVideo, defaultDevice.toStdString());
        ret.reserve(devices.size());
        std::transform(devices.cbegin(),
                       devices.cend(),
                       std::back_inserter(ret),
                       [](const auto &d) { return QString::fromStdString(d); });

        return ret;
}

void
CallManager::generateCallID()
{
        using namespace std::chrono;
        uint64_t ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        callid_     = "c" + std::to_string(ms);
}

void
CallManager::clear()
{
        roomid_.clear();
        callParty_.clear();
        callPartyAvatarUrl_.clear();
        callid_.clear();
        callType_       = CallType::VOICE;
        haveCallInvite_ = false;
        emit newInviteState();
        inviteSDP_.clear();
        remoteICECandidates_.clear();
}

void
CallManager::endCall()
{
        stopRingtone();
        session_.end();
        clear();
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
CallManager::playRingtone(const QUrl &ringtone, bool repeat)
{
        static QMediaPlaylist playlist;
        playlist.clear();
        playlist.setPlaybackMode(repeat ? QMediaPlaylist::CurrentItemInLoop
                                        : QMediaPlaylist::CurrentItemOnce);
        playlist.addMedia(ringtone);
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
                } else {
                        std::string scheme = std::string(uri, 0, c);
                        if (scheme != "turn" && scheme != "turns") {
                                nhlog::ui()->error("Invalid TURN server uri: {}", uri);
                                continue;
                        }

                        QString encodedUri =
                          QString::fromStdString(scheme) + "://" +
                          QUrl::toPercentEncoding(QString::fromStdString(turnServer.username)) +
                          ":" +
                          QUrl::toPercentEncoding(QString::fromStdString(turnServer.password)) +
                          "@" + QString::fromStdString(std::string(uri, ++c));
                        ret.push_back(encodedUri.toStdString());
                }
        }
        return ret;
}
}
