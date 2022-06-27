// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include <QObject>

#include "mtx/events/voip.hpp"

typedef struct _GstElement GstElement;
class CallDevices;
class QQuickItem;

namespace webrtc {
Q_NAMESPACE

enum class CallType
{
    VOICE,
    VIDEO,
    SCREEN // localUser is sharing screen
};
Q_ENUM_NS(CallType)

enum class State
{
    DISCONNECTED,
    ICEFAILED,
    INITIATING,
    INITIATED,
    OFFERSENT,
    ANSWERSENT,
    CONNECTING,
    CONNECTED

};
Q_ENUM_NS(State)
}

class WebRTCSession : public QObject
{
    Q_OBJECT

public:
    static WebRTCSession &instance()
    {
        static WebRTCSession instance;
        return instance;
    }

    bool havePlugins(bool isVideo, std::string *errorMessage = nullptr);
    webrtc::CallType callType() const { return callType_; }
    webrtc::State state() const { return state_; }
    bool haveLocalPiP() const;
    bool isOffering() const { return isOffering_; }
    bool isRemoteVideoRecvOnly() const { return isRemoteVideoRecvOnly_; }
    bool isRemoteVideoSendOnly() const { return isRemoteVideoSendOnly_; }

    bool createOffer(webrtc::CallType, uint32_t shareWindowId);
    bool acceptOffer(const std::string &sdp);
    bool acceptAnswer(const std::string &sdp);
    void acceptICECandidates(const std::vector<mtx::events::voip::CallCandidates::Candidate> &);

    bool isMicMuted() const;
    bool toggleMicMute();
    void toggleLocalPiP();
    void end();

    void setTurnServers(const std::vector<std::string> &uris) { turnServers_ = uris; }

    void setVideoItem(QQuickItem *item) { videoItem_ = item; }
    QQuickItem *getVideoItem() const { return videoItem_; }

signals:
    void offerCreated(const std::string &sdp,
                      const std::vector<mtx::events::voip::CallCandidates::Candidate> &);
    void answerCreated(const std::string &sdp,
                       const std::vector<mtx::events::voip::CallCandidates::Candidate> &);
    void newICECandidate(const mtx::events::voip::CallCandidates::Candidate &);
    void stateChanged(webrtc::State);

private slots:
    void setState(webrtc::State state) { state_ = state; }

private:
    WebRTCSession();

    CallDevices &devices_;
    bool initialised_           = false;
    bool haveVoicePlugins_      = false;
    bool haveVideoPlugins_      = false;
    webrtc::CallType callType_  = webrtc::CallType::VOICE;
    webrtc::State state_        = webrtc::State::DISCONNECTED;
    bool isOffering_            = false;
    bool isRemoteVideoRecvOnly_ = false;
    bool isRemoteVideoSendOnly_ = false;
    QQuickItem *videoItem_      = nullptr;
    GstElement *pipe_           = nullptr;
    GstElement *webrtc_         = nullptr;
    unsigned int busWatchId_    = 0;
    std::vector<std::string> turnServers_;
    uint32_t shareWindowId_ = 0;

    bool init(std::string *errorMessage = nullptr);
    bool startPipeline(int opusPayloadType, int vp8PayloadType);
    bool createPipeline(int opusPayloadType, int vp8PayloadType);
    bool addVideoPipeline(int vp8PayloadType);
    void clear();

public:
    WebRTCSession(WebRTCSession const &) = delete;
    void operator=(WebRTCSession const &) = delete;
};
