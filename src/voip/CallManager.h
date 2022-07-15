// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

#include "CallDevices.h"
#include "WebRTCSession.h"
#include "mtx/events/collections.hpp"
#include "mtx/events/voip.hpp"

namespace mtx::responses {
struct TurnServer;
}

class QUrl;

class CallManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool haveCallInvite READ haveCallInvite NOTIFY newInviteState)
    Q_PROPERTY(bool isOnCall READ isOnCall NOTIFY newCallState)
    Q_PROPERTY(webrtc::CallType callType READ callType NOTIFY newInviteState)
    Q_PROPERTY(webrtc::State callState READ callState NOTIFY newCallState)
    Q_PROPERTY(QString callParty READ callParty NOTIFY newInviteState)
    Q_PROPERTY(QString callPartyDisplayName READ callPartyDisplayName NOTIFY newInviteState)
    Q_PROPERTY(QString callPartyAvatarUrl READ callPartyAvatarUrl NOTIFY newInviteState)
    Q_PROPERTY(bool isMicMuted READ isMicMuted NOTIFY micMuteChanged)
    Q_PROPERTY(bool haveLocalPiP READ haveLocalPiP NOTIFY newCallState)
    Q_PROPERTY(QStringList mics READ mics NOTIFY devicesChanged)
    Q_PROPERTY(QStringList cameras READ cameras NOTIFY devicesChanged)
    Q_PROPERTY(bool callsSupported READ callsSupported CONSTANT)
    Q_PROPERTY(bool screenShareSupported READ screenShareSupported CONSTANT)

public:
    CallManager(QObject *);

    bool haveCallInvite() const { return haveCallInvite_; }
    bool isOnCall() const { return session_.state() != webrtc::State::DISCONNECTED; }
    webrtc::CallType callType() const { return callType_; }
    webrtc::State callState() const { return session_.state(); }
    QString callParty() const { return callParty_; }
    QString callPartyDisplayName() const { return callPartyDisplayName_; }
    QString callPartyAvatarUrl() const { return callPartyAvatarUrl_; }
    bool isMicMuted() const { return session_.isMicMuted(); }
    bool haveLocalPiP() const { return session_.haveLocalPiP(); }
    QStringList mics() const { return devices(false); }
    QStringList cameras() const { return devices(true); }
    void refreshTurnServer();

    static bool callsSupported();
    static bool screenShareSupported();

public slots:
    void sendInvite(const QString &roomid, webrtc::CallType, unsigned int windowIndex = 0);
    void syncEvent(const mtx::events::collections::TimelineEvents &event);
    void toggleMicMute();
    void toggleLocalPiP() { session_.toggleLocalPiP(); }
    void acceptInvite();
    void
      hangUp(mtx::events::voip::CallHangUp::Reason = mtx::events::voip::CallHangUp::Reason::User);
    QStringList windowList();
    void previewWindow(unsigned int windowIndex) const;

signals:
    void newMessage(const QString &roomid, const mtx::events::voip::CallInvite &);
    void newMessage(const QString &roomid, const mtx::events::voip::CallCandidates &);
    void newMessage(const QString &roomid, const mtx::events::voip::CallAnswer &);
    void newMessage(const QString &roomid, const mtx::events::voip::CallHangUp &);
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
    QString callPartyDisplayName_;
    QString callPartyAvatarUrl_;
    std::string callid_;
    std::string partyid_       = "";
    std::string invitee_       = "";
    const uint32_t timeoutms_  = 120000;
    webrtc::CallType callType_ = webrtc::CallType::VOICE;
    bool haveCallInvite_       = false;
    std::string inviteSDP_;
    std::vector<mtx::events::voip::CallCandidates::Candidate> remoteICECandidates_;
    std::vector<std::string> turnURIs_;
    QTimer turnServerTimer_;
    QMediaPlayer player_;
    std::vector<std::pair<QString, uint32_t>> windows_;

    template<typename T>
    bool handleEvent(const mtx::events::collections::TimelineEvents &event);
    void handleEvent(const mtx::events::RoomEvent<mtx::events::voip::CallInvite> &);
    void handleEvent(const mtx::events::RoomEvent<mtx::events::voip::CallCandidates> &);
    void handleEvent(const mtx::events::RoomEvent<mtx::events::voip::CallAnswer> &);
    void handleEvent(const mtx::events::RoomEvent<mtx::events::voip::CallHangUp> &);
    void answerInvite(const mtx::events::voip::CallInvite &, bool isVideo);
    void generateCallID();
    QStringList devices(bool isVideo) const;
    void clear();
    void endCall();
    void playRingtone(const QUrl &ringtone, bool repeat);
    void stopRingtone();
};
