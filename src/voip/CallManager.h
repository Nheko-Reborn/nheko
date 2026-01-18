// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include <QMediaPlayer>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QTimer>

#include "CallDevices.h"
#include "WebRTCSession.h"
#include "mtx/events/collections.hpp"
#include "mtx/events/voip.hpp"
#include "voip/ScreenCastPortal.h"
#include <mtxclient/utils.hpp>

namespace mtx::responses {
struct TurnServer;
}

class QUrl;

class CallManager : public QObject
{
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool haveCallInvite READ haveCallInvite NOTIFY newInviteState)
    Q_PROPERTY(bool isOnCall READ isOnCall NOTIFY newCallState)
    Q_PROPERTY(bool isOnCallOnOtherDevice READ isOnCallOnOtherDevice NOTIFY newCallDeviceState)
    Q_PROPERTY(webrtc::CallType callType READ callType NOTIFY newInviteState)
    Q_PROPERTY(
      webrtc::ScreenShareType screenShareType READ screenShareType NOTIFY screenShareChanged)
    Q_PROPERTY(webrtc::State callState READ callState NOTIFY newCallState)
    Q_PROPERTY(QString callParty READ callParty NOTIFY newInviteState)
    Q_PROPERTY(QString callPartyDisplayName READ callPartyDisplayName NOTIFY newInviteState)
    Q_PROPERTY(QString callPartyAvatarUrl READ callPartyAvatarUrl NOTIFY newInviteState)
    Q_PROPERTY(bool isMicMuted READ isMicMuted NOTIFY micMuteChanged)
    Q_PROPERTY(bool haveLocalPiP READ haveLocalPiP NOTIFY newCallState)
    Q_PROPERTY(QStringList mics READ mics NOTIFY devicesChanged)
    Q_PROPERTY(QStringList cameras READ cameras NOTIFY devicesChanged)
    Q_PROPERTY(bool callsSupported READ callsSupported CONSTANT)
    Q_PROPERTY(bool screenShareReady READ screenShareReady NOTIFY screenShareChanged)

public:
    CallManager(QObject *);

    static CallManager *create(QQmlEngine *qmlEngine, QJSEngine *);

    bool haveCallInvite() const { return haveCallInvite_; }
    bool isOnCall() const { return (session_.state() != webrtc::State::DISCONNECTED); }
    bool isOnCallOnOtherDevice() const { return (isOnCallOnOtherDevice_ != ""); }
    bool checkSharesRoom(QString roomid_, std::string invitee) const;
    webrtc::CallType callType() const { return callType_; }
    webrtc::ScreenShareType screenShareType() const { return screenShareType_; }
    webrtc::State callState() const { return session_.state(); }
    QString callParty() const { return callParty_; }
    QString callPartyDisplayName() const { return callPartyDisplayName_; }
    QString callPartyAvatarUrl() const { return callPartyAvatarUrl_; }
    bool isMicMuted() const { return session_.isMicMuted(); }
    bool haveLocalPiP() const { return session_.haveLocalPiP(); }
    QStringList mics() const { return devices(false); }
    QStringList cameras() const { return devices(true); }
    void refreshTurnServer();
    bool screenShareReady() const;

    static bool callsSupported();

public slots:
    void sendInvite(const QString &roomid, webrtc::CallType, unsigned int windowIndex = 0);
    void syncEvent(const mtx::events::collections::TimelineEvents &event);
    void toggleMicMute();
    void toggleLocalPiP() { session_.toggleLocalPiP(); }
    void acceptInvite();
    void hangUp(
      mtx::events::voip::CallHangUp::Reason = mtx::events::voip::CallHangUp::Reason::UserHangUp);
    void rejectInvite();
    void setupScreenShareXDP();
    void setScreenShareType(unsigned int index);
    void closeScreenShare();
    QStringList screenShareTypeList();
    QStringList windowList();
    void previewWindow(unsigned int windowIndex) const;
    void refreshDevices();

signals:
    void newMessage(const QString &roomid, const mtx::events::voip::CallInvite &);
    void newMessage(const QString &roomid, const mtx::events::voip::CallCandidates &);
    void newMessage(const QString &roomid, const mtx::events::voip::CallAnswer &);
    void newMessage(const QString &roomid, const mtx::events::voip::CallHangUp &);
    void newMessage(const QString &roomid, const mtx::events::voip::CallSelectAnswer &);
    void newMessage(const QString &roomid, const mtx::events::voip::CallReject &);
    void newMessage(const QString &roomid, const mtx::events::voip::CallNegotiate &);
    void newInviteState();
    void newCallState();
    void newCallDeviceState();
    void micMuteChanged();
    void devicesChanged();
    void turnServerRetrieved(const mtx::responses::TurnServer &);
    void screenShareChanged();

private slots:
    void retrieveTurnServer();

private:
    WebRTCSession &session_;
    QString roomid_;
    QString callParty_;
    QString callPartyDisplayName_;
    QString callPartyAvatarUrl_;
    std::string callPartyVersion_ = "1";
    std::string callid_;
    std::string partyid_                     = mtx::client::utils::random_token(8, false);
    std::string selectedpartyid_             = "";
    std::string invitee_                     = "";
    const uint32_t timeoutms_                = 120000;
    webrtc::CallType callType_               = webrtc::CallType::VOICE;
    webrtc::ScreenShareType screenShareType_ = webrtc::ScreenShareType::X11;
    bool haveCallInvite_                     = false;
    bool answerSelected_                     = false;
    std::string isOnCallOnOtherDevice_       = "";
    std::string inviteSDP_;
    std::vector<mtx::events::voip::CallCandidates::Candidate> remoteICECandidates_;
    std::vector<std::string> turnURIs_;
    QTimer turnServerTimer_;
    QMediaPlayer player_;
    std::vector<webrtc::ScreenShareType> screenShareTypes_;
#ifndef Q_OS_WINDOWS
    std::vector<std::pair<QString, uint32_t>> windows_;
#else
    std::vector<std::pair<QString, uint64_t>> windows_;
#endif
    std::vector<std::string> rejectCallPartyIDs_;

    template<typename T>
    bool handleEvent(const mtx::events::collections::TimelineEvents &event);
    void handleEvent(const mtx::events::RoomEvent<mtx::events::voip::CallInvite> &);
    void handleEvent(const mtx::events::RoomEvent<mtx::events::voip::CallCandidates> &);
    void handleEvent(const mtx::events::RoomEvent<mtx::events::voip::CallAnswer> &);
    void handleEvent(const mtx::events::RoomEvent<mtx::events::voip::CallHangUp> &);
    void handleEvent(const mtx::events::RoomEvent<mtx::events::voip::CallSelectAnswer> &);
    void handleEvent(const mtx::events::RoomEvent<mtx::events::voip::CallReject> &);
    void handleEvent(const mtx::events::RoomEvent<mtx::events::voip::CallNegotiate> &);
    void answerInvite(const mtx::events::voip::CallInvite &, bool isVideo);
    void generateCallID();
    QStringList devices(bool isVideo) const;
    void clear(bool endAllCalls = true);
    void endCall(bool endAllCalls = true);
    void playRingtone(const QUrl &ringtone, bool repeat);
    void stopRingtone();
};
