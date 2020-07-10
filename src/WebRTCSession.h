#pragma once

#include <string>
#include <vector>

#include <QObject>

#include "mtx/events/voip.hpp"

typedef struct _GstElement GstElement;

class WebRTCSession : public QObject
{
        Q_OBJECT

public:
        static WebRTCSession& instance()
        {
          static WebRTCSession instance;
          return instance;
        }

        bool init(std::string *errorMessage = nullptr);

        bool createOffer();
        bool acceptOffer(const std::string &sdp);
        bool acceptAnswer(const std::string &sdp);
        void acceptICECandidates(const std::vector<mtx::events::msg::CallCandidates::Candidate>&);

        bool isActive() { return pipe_ != nullptr; }
        bool toggleMuteAudioSrc(bool &isMuted);
        void end();

        void setStunServer(const std::string &stunServer) {stunServer_ = stunServer;}
        void setTurnServers(const std::vector<std::string> &uris) {turnServers_ = uris;}

signals:
        void offerCreated(const std::string &sdp, const std::vector<mtx::events::msg::CallCandidates::Candidate>&);
        void answerCreated(const std::string &sdp, const std::vector<mtx::events::msg::CallCandidates::Candidate>&);
        void pipelineChanged(bool started);

private:
        WebRTCSession() : QObject() {}

        bool initialised_ = false;
        GstElement *pipe_ = nullptr;
        GstElement *webrtc_ = nullptr;
        std::string stunServer_;
        std::vector<std::string> turnServers_;

        bool startPipeline(int opusPayloadType);
        bool createPipeline(int opusPayloadType);
        void addTurnServers();

public:
        WebRTCSession(WebRTCSession const&) = delete;
        void operator=(WebRTCSession const&) = delete;
};
