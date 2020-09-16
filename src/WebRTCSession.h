#pragma once

#include <string>
#include <vector>

#include <QObject>

#include "mtx/events/voip.hpp"

typedef struct _GList GList;
typedef struct _GstElement GstElement;

class WebRTCSession : public QObject
{
        Q_OBJECT

public:
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

        static WebRTCSession &instance()
        {
                static WebRTCSession instance;
                return instance;
        }

        bool init(std::string *errorMessage = nullptr);
        State state() const { return state_; }

        bool createOffer();
        bool acceptOffer(const std::string &sdp);
        bool acceptAnswer(const std::string &sdp);
        void acceptICECandidates(const std::vector<mtx::events::msg::CallCandidates::Candidate> &);

        bool toggleMuteAudioSrc(bool &isMuted);
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
        void stateChanged(WebRTCSession::State); // explicit qualifier necessary for Qt

private slots:
        void setState(State state) { state_ = state; }

private:
        WebRTCSession();

        bool initialised_        = false;
        State state_             = State::DISCONNECTED;
        GstElement *pipe_        = nullptr;
        GstElement *webrtc_      = nullptr;
        unsigned int busWatchId_ = 0;
        std::string stunServer_;
        std::vector<std::string> turnServers_;
        GList *audioSources_  = nullptr;
        int audioSourceIndex_ = -1;

        bool startPipeline(int opusPayloadType);
        bool createPipeline(int opusPayloadType);
        void refreshDevices();

public:
        WebRTCSession(WebRTCSession const &) = delete;
        void operator=(WebRTCSession const &) = delete;
};
