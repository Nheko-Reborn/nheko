#pragma once

#include <string>
#include <vector>

#include <QObject>
#include <QSharedPointer>

#include "mtx/events/voip.hpp"

typedef struct _GstElement GstElement;
class QQuickItem;
class UserSettings;

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

        bool havePlugins(bool isVideo, std::string *errorMessage = nullptr);
        webrtc::State state() const { return state_; }
        bool isVideo() const { return isVideo_; }
        bool isOffering() const { return isOffering_; }
        bool isRemoteVideoRecvOnly() const { return isRemoteVideoRecvOnly_; }

        bool createOffer(bool isVideo);
        bool acceptOffer(const std::string &sdp);
        bool acceptAnswer(const std::string &sdp);
        void acceptICECandidates(const std::vector<mtx::events::msg::CallCandidates::Candidate> &);

        bool isMicMuted() const;
        bool toggleMicMute();
        void end();

        void setSettings(QSharedPointer<UserSettings> settings) { settings_ = settings; }
        void setTurnServers(const std::vector<std::string> &uris) { turnServers_ = uris; }

        void refreshDevices();
        std::vector<std::string> getDeviceNames(bool isVideo,
                                                const std::string &defaultDevice) const;
        std::vector<std::string> getResolutions(const std::string &cameraName) const;
        std::vector<std::string> getFrameRates(const std::string &cameraName,
                                               const std::string &resolution) const;

        void setVideoItem(QQuickItem *item) { videoItem_ = item; }
        QQuickItem *getVideoItem() const { return videoItem_; }

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

        bool initialised_           = false;
        bool haveVoicePlugins_      = false;
        bool haveVideoPlugins_      = false;
        webrtc::State state_        = webrtc::State::DISCONNECTED;
        bool isVideo_               = false;
        bool isOffering_            = false;
        bool isRemoteVideoRecvOnly_ = false;
        QQuickItem *videoItem_      = nullptr;
        GstElement *pipe_           = nullptr;
        GstElement *webrtc_         = nullptr;
        unsigned int busWatchId_    = 0;
        QSharedPointer<UserSettings> settings_;
        std::vector<std::string> turnServers_;

        bool init(std::string *errorMessage = nullptr);
        bool startPipeline(int opusPayloadType, int vp8PayloadType);
        bool createPipeline(int opusPayloadType, int vp8PayloadType);
        bool addVideoPipeline(int vp8PayloadType);
        void startDeviceMonitor();

public:
        WebRTCSession(WebRTCSession const &) = delete;
        void operator=(WebRTCSession const &) = delete;
};
