#pragma once

#include <string>
#include <vector>

#include <QObject>

#include "mtx/events/voip.hpp"

typedef struct _GstElement GstElement;

namespace webrtc {
Q_NAMESPACE

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

        bool init(std::string *errorMessage = nullptr);
        webrtc::State state() const { return state_; }

        bool createOffer();
        bool acceptOffer(const std::string &sdp);
        bool acceptAnswer(const std::string &sdp);
        void acceptICECandidates(const std::vector<mtx::events::msg::CallCandidates::Candidate> &);

        bool toggleMuteAudioSource();
        void end();

        void setStunServer(const std::string &stunServer) { stunServer_ = stunServer; }
        void setTurnServers(const std::vector<std::string> &uris) { turnServers_ = uris; }

        std::vector<std::string> getAudioSourceNames(const std::string &defaultDevice);
        void setAudioSource(int audioDeviceIndex) { audioSourceIndex_ = audioDeviceIndex; }

signals:
        void offerCreated(const std::string &sdp,
                          const std::vector<mtx::events::msg::CallCandidates::Candidate> &);
        void answerCreated(const std::string &sdp,
                           const std::vector<mtx::events::msg::CallCandidates::Candidate> &);
        void newICECandidate(const mtx::events::msg::CallCandidates::Candidate &);
        void stateChanged(webrtc::State);

private slots:
        void setState(webrtc::State state) { state_ = state; }

private:
        WebRTCSession();

        bool initialised_        = false;
        webrtc::State state_     = webrtc::State::DISCONNECTED;
        GstElement *pipe_        = nullptr;
        GstElement *webrtc_      = nullptr;
        unsigned int busWatchId_ = 0;
        std::string stunServer_;
        std::vector<std::string> turnServers_;
        int audioSourceIndex_ = -1;

        bool startPipeline(int opusPayloadType);
        bool createPipeline(int opusPayloadType);
        void refreshDevices();
        void startDeviceMonitor();

public:
        WebRTCSession(WebRTCSession const &) = delete;
        void operator=(WebRTCSession const &) = delete;
};
