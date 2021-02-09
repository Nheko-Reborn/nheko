#pragma once

#include <string>
#include <vector>

#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QTimer>

#include "CallDevices.h"
#include "WebRTCSession.h"
#include "mtx/events/collections.hpp"
#include "mtx/events/voip.hpp"

namespace mtx::responses {
struct TurnServer;
}

class QStringList;
class QUrl;

class CallManager : public QObject
{
        Q_OBJECT
        Q_PROPERTY(bool haveCallInvite READ haveCallInvite NOTIFY newInviteState)
        Q_PROPERTY(bool isOnCall READ isOnCall NOTIFY newCallState)
        Q_PROPERTY(bool isVideo READ isVideo NOTIFY newInviteState)
        Q_PROPERTY(bool haveLocalVideo READ haveLocalVideo NOTIFY newCallState)
        Q_PROPERTY(webrtc::State callState READ callState NOTIFY newCallState)
        Q_PROPERTY(QString callParty READ callParty NOTIFY newInviteState)
        Q_PROPERTY(QString callPartyAvatarUrl READ callPartyAvatarUrl NOTIFY newInviteState)
        Q_PROPERTY(bool isMicMuted READ isMicMuted NOTIFY micMuteChanged)
        Q_PROPERTY(bool callsSupported READ callsSupported CONSTANT)
        Q_PROPERTY(QStringList mics READ mics NOTIFY devicesChanged)
        Q_PROPERTY(QStringList cameras READ cameras NOTIFY devicesChanged)

public:
        CallManager(QObject *);

        bool haveCallInvite() const { return haveCallInvite_; }
        bool isOnCall() const { return session_.state() != webrtc::State::DISCONNECTED; }
        bool isVideo() const { return isVideo_; }
        bool haveLocalVideo() const { return session_.haveLocalVideo(); }
        webrtc::State callState() const { return session_.state(); }
        QString callParty() const { return callParty_; }
        QString callPartyAvatarUrl() const { return callPartyAvatarUrl_; }
        bool isMicMuted() const { return session_.isMicMuted(); }
        bool callsSupported() const;
        QStringList mics() const { return devices(false); }
        QStringList cameras() const { return devices(true); }
        void refreshTurnServer();

public slots:
        void sendInvite(const QString &roomid, bool isVideo);
        void syncEvent(const mtx::events::collections::TimelineEvents &event);
        void refreshDevices() { CallDevices::instance().refresh(); }
        void toggleMicMute();
        void toggleCameraView() { session_.toggleCameraView(); }
        void acceptInvite();
        void hangUp(
          mtx::events::msg::CallHangUp::Reason = mtx::events::msg::CallHangUp::Reason::User);

signals:
        void newMessage(const QString &roomid, const mtx::events::msg::CallInvite &);
        void newMessage(const QString &roomid, const mtx::events::msg::CallCandidates &);
        void newMessage(const QString &roomid, const mtx::events::msg::CallAnswer &);
        void newMessage(const QString &roomid, const mtx::events::msg::CallHangUp &);
        void newInviteState();
        void newCallState();
        void micMuteChanged();
        void devicesChanged();
        void turnServerRetrieved(const mtx::responses::TurnServer &);

private slots:
        void retrieveTurnServer();

private:
        WebRTCSession &session_;
        QString roomid_;
        QString callParty_;
        QString callPartyAvatarUrl_;
        std::string callid_;
        const uint32_t timeoutms_ = 120000;
        bool isVideo_             = false;
        bool haveCallInvite_      = false;
        std::string inviteSDP_;
        std::vector<mtx::events::msg::CallCandidates::Candidate> remoteICECandidates_;
        std::vector<std::string> turnURIs_;
        QTimer turnServerTimer_;
        QMediaPlayer player_;

        template<typename T>
        bool handleEvent_(const mtx::events::collections::TimelineEvents &event);
        void handleEvent(const mtx::events::RoomEvent<mtx::events::msg::CallInvite> &);
        void handleEvent(const mtx::events::RoomEvent<mtx::events::msg::CallCandidates> &);
        void handleEvent(const mtx::events::RoomEvent<mtx::events::msg::CallAnswer> &);
        void handleEvent(const mtx::events::RoomEvent<mtx::events::msg::CallHangUp> &);
        void answerInvite(const mtx::events::msg::CallInvite &, bool isVideo);
        void generateCallID();
        QStringList devices(bool isVideo) const;
        void clear();
        void endCall();
        void playRingtone(const QUrl &ringtone, bool repeat);
        void stopRingtone();
};
