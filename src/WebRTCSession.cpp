#include <cctype>

#include "Logging.h"
#include "WebRTCSession.h"

#ifdef GSTREAMER_AVAILABLE
extern "C"
{
#include "gst/gst.h"
#include "gst/sdp/sdp.h"

#define GST_USE_UNSTABLE_API
#include "gst/webrtc/webrtc.h"
}
#endif

Q_DECLARE_METATYPE(WebRTCSession::State)

WebRTCSession::WebRTCSession()
  : QObject()
{
        qRegisterMetaType<WebRTCSession::State>();
        connect(this, &WebRTCSession::stateChanged, this, &WebRTCSession::setState);
}

bool
WebRTCSession::init(std::string *errorMessage)
{
#ifdef GSTREAMER_AVAILABLE
        if (initialised_)
                return true;

        GError *error = nullptr;
        if (!gst_init_check(nullptr, nullptr, &error)) {
                std::string strError = std::string("WebRTC: failed to initialise GStreamer: ");
                if (error) {
                        strError += error->message;
                        g_error_free(error);
                }
                nhlog::ui()->error(strError);
                if (errorMessage)
                        *errorMessage = strError;
                return false;
        }

        gchar *version = gst_version_string();
        std::string gstVersion(version);
        g_free(version);
        nhlog::ui()->info("WebRTC: initialised " + gstVersion);

        // GStreamer Plugins:
        // Base:            audioconvert, audioresample, opus, playback, volume
        // Good:            autodetect, rtpmanager
        // Bad:             dtls, srtp, webrtc
        // libnice [GLib]:  nice
        initialised_          = true;
        std::string strError  = gstVersion + ": Missing plugins: ";
        const gchar *needed[] = {"audioconvert",
                                 "audioresample",
                                 "autodetect",
                                 "dtls",
                                 "nice",
                                 "opus",
                                 "playback",
                                 "rtpmanager",
                                 "srtp",
                                 "volume",
                                 "webrtc",
                                 nullptr};
        GstRegistry *registry = gst_registry_get();
        for (guint i = 0; i < g_strv_length((gchar **)needed); i++) {
                GstPlugin *plugin = gst_registry_find_plugin(registry, needed[i]);
                if (!plugin) {
                        strError += std::string(needed[i]) + " ";
                        initialised_ = false;
                        continue;
                }
                gst_object_unref(plugin);
        }

        if (!initialised_) {
                nhlog::ui()->error(strError);
                if (errorMessage)
                        *errorMessage = strError;
        }
        return initialised_;
#else
        (void)errorMessage;
        return false;
#endif
}

#ifdef GSTREAMER_AVAILABLE
namespace {
bool isoffering_;
std::string localsdp_;
std::vector<mtx::events::msg::CallCandidates::Candidate> localcandidates_;

gboolean
newBusMessage(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, gpointer user_data)
{
        WebRTCSession *session = static_cast<WebRTCSession *>(user_data);
        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
                nhlog::ui()->error("WebRTC: end of stream");
                session->end();
                break;
        case GST_MESSAGE_ERROR:
                GError *error;
                gchar *debug;
                gst_message_parse_error(msg, &error, &debug);
                nhlog::ui()->error(
                  "WebRTC: error from element {}: {}", GST_OBJECT_NAME(msg->src), error->message);
                g_clear_error(&error);
                g_free(debug);
                session->end();
                break;
        default:
                break;
        }
        return TRUE;
}

GstWebRTCSessionDescription *
parseSDP(const std::string &sdp, GstWebRTCSDPType type)
{
        GstSDPMessage *msg;
        gst_sdp_message_new(&msg);
        if (gst_sdp_message_parse_buffer((guint8 *)sdp.c_str(), sdp.size(), msg) == GST_SDP_OK) {
                return gst_webrtc_session_description_new(type, msg);
        } else {
                nhlog::ui()->error("WebRTC: failed to parse remote session description");
                gst_object_unref(msg);
                return nullptr;
        }
}

void
setLocalDescription(GstPromise *promise, gpointer webrtc)
{
        const GstStructure *reply = gst_promise_get_reply(promise);
        gboolean isAnswer = gst_structure_id_has_field(reply, g_quark_from_string("answer"));
        GstWebRTCSessionDescription *gstsdp = nullptr;
        gst_structure_get(reply,
                          isAnswer ? "answer" : "offer",
                          GST_TYPE_WEBRTC_SESSION_DESCRIPTION,
                          &gstsdp,
                          nullptr);
        gst_promise_unref(promise);
        g_signal_emit_by_name(webrtc, "set-local-description", gstsdp, nullptr);

        gchar *sdp = gst_sdp_message_as_text(gstsdp->sdp);
        localsdp_  = std::string(sdp);
        g_free(sdp);
        gst_webrtc_session_description_free(gstsdp);

        nhlog::ui()->debug(
          "WebRTC: local description set ({}):\n{}", isAnswer ? "answer" : "offer", localsdp_);
}

void
createOffer(GstElement *webrtc)
{
        // create-offer first, then set-local-description
        GstPromise *promise =
          gst_promise_new_with_change_func(setLocalDescription, webrtc, nullptr);
        g_signal_emit_by_name(webrtc, "create-offer", nullptr, promise);
}

void
createAnswer(GstPromise *promise, gpointer webrtc)
{
        // create-answer first, then set-local-description
        gst_promise_unref(promise);
        promise = gst_promise_new_with_change_func(setLocalDescription, webrtc, nullptr);
        g_signal_emit_by_name(webrtc, "create-answer", nullptr, promise);
}

#if GST_CHECK_VERSION(1, 18, 0)
void
iceGatheringStateChanged(GstElement *webrtc,
                         GParamSpec *pspec G_GNUC_UNUSED,
                         gpointer user_data G_GNUC_UNUSED)
{
        GstWebRTCICEGatheringState newState;
        g_object_get(webrtc, "ice-gathering-state", &newState, nullptr);
        if (newState == GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE) {
                nhlog::ui()->debug("WebRTC: GstWebRTCICEGatheringState -> Complete");
                if (isoffering_) {
                        emit WebRTCSession::instance().offerCreated(localsdp_, localcandidates_);
                        emit WebRTCSession::instance().stateChanged(
                          WebRTCSession::State::OFFERSENT);
                } else {
                        emit WebRTCSession::instance().answerCreated(localsdp_, localcandidates_);
                        emit WebRTCSession::instance().stateChanged(
                          WebRTCSession::State::ANSWERSENT);
                }
        }
}

#else

gboolean
onICEGatheringCompletion(gpointer timerid)
{
        *(guint *)(timerid) = 0;
        if (isoffering_) {
                emit WebRTCSession::instance().offerCreated(localsdp_, localcandidates_);
                emit WebRTCSession::instance().stateChanged(WebRTCSession::State::OFFERSENT);
        } else {
                emit WebRTCSession::instance().answerCreated(localsdp_, localcandidates_);
                emit WebRTCSession::instance().stateChanged(WebRTCSession::State::ANSWERSENT);
        }
        return FALSE;
}
#endif

void
addLocalICECandidate(GstElement *webrtc G_GNUC_UNUSED,
                     guint mlineIndex,
                     gchar *candidate,
                     gpointer G_GNUC_UNUSED)
{
        nhlog::ui()->debug("WebRTC: local candidate: (m-line:{}):{}", mlineIndex, candidate);

#if GST_CHECK_VERSION(1, 18, 0)
        localcandidates_.push_back({"audio", (uint16_t)mlineIndex, candidate});
        return;
#else
        if (WebRTCSession::instance().state() >= WebRTCSession::State::OFFERSENT) {
                emit WebRTCSession::instance().newICECandidate(
                  {"audio", (uint16_t)mlineIndex, candidate});
                return;
        }

        localcandidates_.push_back({"audio", (uint16_t)mlineIndex, candidate});

        // GStreamer v1.16: webrtcbin's notify::ice-gathering-state triggers
        // GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE too early. Fixed in v1.18.
        // Use a 100ms timeout in the meantime
        static guint timerid = 0;
        if (timerid)
                g_source_remove(timerid);

        timerid = g_timeout_add(100, onICEGatheringCompletion, &timerid);
#endif
}

void
iceConnectionStateChanged(GstElement *webrtc,
                          GParamSpec *pspec G_GNUC_UNUSED,
                          gpointer user_data G_GNUC_UNUSED)
{
        GstWebRTCICEConnectionState newState;
        g_object_get(webrtc, "ice-connection-state", &newState, nullptr);
        switch (newState) {
        case GST_WEBRTC_ICE_CONNECTION_STATE_CHECKING:
                nhlog::ui()->debug("WebRTC: GstWebRTCICEConnectionState -> Checking");
                emit WebRTCSession::instance().stateChanged(WebRTCSession::State::CONNECTING);
                break;
        case GST_WEBRTC_ICE_CONNECTION_STATE_FAILED:
                nhlog::ui()->error("WebRTC: GstWebRTCICEConnectionState -> Failed");
                emit WebRTCSession::instance().stateChanged(WebRTCSession::State::ICEFAILED);
                break;
        default:
                break;
        }
}

void
linkNewPad(GstElement *decodebin G_GNUC_UNUSED, GstPad *newpad, GstElement *pipe)
{
        GstCaps *caps = gst_pad_get_current_caps(newpad);
        if (!caps)
                return;

        const gchar *name = gst_structure_get_name(gst_caps_get_structure(caps, 0));
        gst_caps_unref(caps);

        GstPad *queuepad = nullptr;
        if (g_str_has_prefix(name, "audio")) {
                nhlog::ui()->debug("WebRTC: received incoming audio stream");
                GstElement *queue    = gst_element_factory_make("queue", nullptr);
                GstElement *convert  = gst_element_factory_make("audioconvert", nullptr);
                GstElement *resample = gst_element_factory_make("audioresample", nullptr);
                GstElement *sink     = gst_element_factory_make("autoaudiosink", nullptr);
                gst_bin_add_many(GST_BIN(pipe), queue, convert, resample, sink, nullptr);
                gst_element_link_many(queue, convert, resample, sink, nullptr);
                gst_element_sync_state_with_parent(queue);
                gst_element_sync_state_with_parent(convert);
                gst_element_sync_state_with_parent(resample);
                gst_element_sync_state_with_parent(sink);
                queuepad = gst_element_get_static_pad(queue, "sink");
        }

        if (queuepad) {
                if (GST_PAD_LINK_FAILED(gst_pad_link(newpad, queuepad)))
                        nhlog::ui()->error("WebRTC: unable to link new pad");
                else {
                        emit WebRTCSession::instance().stateChanged(
                          WebRTCSession::State::CONNECTED);
                }
                gst_object_unref(queuepad);
        }
}

void
addDecodeBin(GstElement *webrtc G_GNUC_UNUSED, GstPad *newpad, GstElement *pipe)
{
        if (GST_PAD_DIRECTION(newpad) != GST_PAD_SRC)
                return;

        nhlog::ui()->debug("WebRTC: received incoming stream");
        GstElement *decodebin = gst_element_factory_make("decodebin", nullptr);
        g_signal_connect(decodebin, "pad-added", G_CALLBACK(linkNewPad), pipe);
        gst_bin_add(GST_BIN(pipe), decodebin);
        gst_element_sync_state_with_parent(decodebin);
        GstPad *sinkpad = gst_element_get_static_pad(decodebin, "sink");
        if (GST_PAD_LINK_FAILED(gst_pad_link(newpad, sinkpad)))
                nhlog::ui()->error("WebRTC: unable to link new pad");
        gst_object_unref(sinkpad);
}

std::string::const_iterator
findName(const std::string &sdp, const std::string &name)
{
        return std::search(
          sdp.cbegin(),
          sdp.cend(),
          name.cbegin(),
          name.cend(),
          [](unsigned char c1, unsigned char c2) { return std::tolower(c1) == std::tolower(c2); });
}

int
getPayloadType(const std::string &sdp, const std::string &name)
{
        // eg a=rtpmap:111 opus/48000/2
        auto e = findName(sdp, name);
        if (e == sdp.cend()) {
                nhlog::ui()->error("WebRTC: remote offer - " + name + " attribute missing");
                return -1;
        }

        if (auto s = sdp.rfind(':', e - sdp.cbegin()); s == std::string::npos) {
                nhlog::ui()->error("WebRTC: remote offer - unable to determine " + name +
                                   " payload type");
                return -1;
        } else {
                ++s;
                try {
                        return std::stoi(std::string(sdp, s, e - sdp.cbegin() - s));
                } catch (...) {
                        nhlog::ui()->error("WebRTC: remote offer - unable to determine " + name +
                                           " payload type");
                }
        }
        return -1;
}

}

bool
WebRTCSession::createOffer()
{
        isoffering_ = true;
        localsdp_.clear();
        localcandidates_.clear();
        return startPipeline(111); // a dynamic opus payload type
}

bool
WebRTCSession::acceptOffer(const std::string &sdp)
{
        nhlog::ui()->debug("WebRTC: received offer:\n{}", sdp);
        if (state_ != State::DISCONNECTED)
                return false;

        isoffering_ = false;
        localsdp_.clear();
        localcandidates_.clear();

        int opusPayloadType = getPayloadType(sdp, "opus");
        if (opusPayloadType == -1)
                return false;

        GstWebRTCSessionDescription *offer = parseSDP(sdp, GST_WEBRTC_SDP_TYPE_OFFER);
        if (!offer)
                return false;

        if (!startPipeline(opusPayloadType)) {
                gst_webrtc_session_description_free(offer);
                return false;
        }

        // set-remote-description first, then create-answer
        GstPromise *promise = gst_promise_new_with_change_func(createAnswer, webrtc_, nullptr);
        g_signal_emit_by_name(webrtc_, "set-remote-description", offer, promise);
        gst_webrtc_session_description_free(offer);
        return true;
}

bool
WebRTCSession::acceptAnswer(const std::string &sdp)
{
        nhlog::ui()->debug("WebRTC: received answer:\n{}", sdp);
        if (state_ != State::OFFERSENT)
                return false;

        GstWebRTCSessionDescription *answer = parseSDP(sdp, GST_WEBRTC_SDP_TYPE_ANSWER);
        if (!answer) {
                end();
                return false;
        }

        g_signal_emit_by_name(webrtc_, "set-remote-description", answer, nullptr);
        gst_webrtc_session_description_free(answer);
        return true;
}

void
WebRTCSession::acceptICECandidates(
  const std::vector<mtx::events::msg::CallCandidates::Candidate> &candidates)
{
        if (state_ >= State::INITIATED) {
                for (const auto &c : candidates) {
                        nhlog::ui()->debug(
                          "WebRTC: remote candidate: (m-line:{}):{}", c.sdpMLineIndex, c.candidate);
                        if (!c.candidate.empty()) {
                                g_signal_emit_by_name(webrtc_,
                                                      "add-ice-candidate",
                                                      c.sdpMLineIndex,
                                                      c.candidate.c_str());
                        }
                }
        }
}

bool
WebRTCSession::startPipeline(int opusPayloadType)
{
        if (state_ != State::DISCONNECTED)
                return false;

        emit stateChanged(State::INITIATING);

        if (!createPipeline(opusPayloadType))
                return false;

        webrtc_ = gst_bin_get_by_name(GST_BIN(pipe_), "webrtcbin");

        if (!stunServer_.empty()) {
                nhlog::ui()->info("WebRTC: setting STUN server: {}", stunServer_);
                g_object_set(webrtc_, "stun-server", stunServer_.c_str(), nullptr);
        }

        for (const auto &uri : turnServers_) {
                nhlog::ui()->info("WebRTC: setting TURN server: {}", uri);
                gboolean udata;
                g_signal_emit_by_name(webrtc_, "add-turn-server", uri.c_str(), (gpointer)(&udata));
        }
        if (turnServers_.empty())
                nhlog::ui()->warn("WebRTC: no TURN server provided");

        // generate the offer when the pipeline goes to PLAYING
        if (isoffering_)
                g_signal_connect(
                  webrtc_, "on-negotiation-needed", G_CALLBACK(::createOffer), nullptr);

        // on-ice-candidate is emitted when a local ICE candidate has been gathered
        g_signal_connect(webrtc_, "on-ice-candidate", G_CALLBACK(addLocalICECandidate), nullptr);

        // capture ICE failure
        g_signal_connect(
          webrtc_, "notify::ice-connection-state", G_CALLBACK(iceConnectionStateChanged), nullptr);

        // incoming streams trigger pad-added
        gst_element_set_state(pipe_, GST_STATE_READY);
        g_signal_connect(webrtc_, "pad-added", G_CALLBACK(addDecodeBin), pipe_);

#if GST_CHECK_VERSION(1, 18, 0)
        // capture ICE gathering completion
        g_signal_connect(
          webrtc_, "notify::ice-gathering-state", G_CALLBACK(iceGatheringStateChanged), nullptr);
#endif
        // webrtcbin lifetime is the same as that of the pipeline
        gst_object_unref(webrtc_);

        // start the pipeline
        GstStateChangeReturn ret = gst_element_set_state(pipe_, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
                nhlog::ui()->error("WebRTC: unable to start pipeline");
                end();
                return false;
        }

        GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe_));
        gst_bus_add_watch(bus, newBusMessage, this);
        gst_object_unref(bus);
        emit stateChanged(State::INITIATED);
        return true;
}

bool
WebRTCSession::createPipeline(int opusPayloadType)
{
        int nSources = audioSources_ ? g_list_length(audioSources_) : 0;
        if (nSources == 0) {
                nhlog::ui()->error("WebRTC: no audio sources");
                return false;
        }

        if (audioSourceIndex_ < 0 || audioSourceIndex_ >= nSources) {
                nhlog::ui()->error("WebRTC: invalid audio source index");
                return false;
        }

        GstElement *source = gst_device_create_element(
          GST_DEVICE_CAST(g_list_nth_data(audioSources_, audioSourceIndex_)), nullptr);
        GstElement *volume     = gst_element_factory_make("volume", "srclevel");
        GstElement *convert    = gst_element_factory_make("audioconvert", nullptr);
        GstElement *resample   = gst_element_factory_make("audioresample", nullptr);
        GstElement *queue1     = gst_element_factory_make("queue", nullptr);
        GstElement *opusenc    = gst_element_factory_make("opusenc", nullptr);
        GstElement *rtp        = gst_element_factory_make("rtpopuspay", nullptr);
        GstElement *queue2     = gst_element_factory_make("queue", nullptr);
        GstElement *capsfilter = gst_element_factory_make("capsfilter", nullptr);

        GstCaps *rtpcaps = gst_caps_new_simple("application/x-rtp",
                                               "media",
                                               G_TYPE_STRING,
                                               "audio",
                                               "encoding-name",
                                               G_TYPE_STRING,
                                               "OPUS",
                                               "payload",
                                               G_TYPE_INT,
                                               opusPayloadType,
                                               nullptr);
        g_object_set(capsfilter, "caps", rtpcaps, nullptr);
        gst_caps_unref(rtpcaps);

        GstElement *webrtcbin = gst_element_factory_make("webrtcbin", "webrtcbin");
        g_object_set(webrtcbin, "bundle-policy", GST_WEBRTC_BUNDLE_POLICY_MAX_BUNDLE, nullptr);

        pipe_ = gst_pipeline_new(nullptr);
        gst_bin_add_many(GST_BIN(pipe_),
                         source,
                         volume,
                         convert,
                         resample,
                         queue1,
                         opusenc,
                         rtp,
                         queue2,
                         capsfilter,
                         webrtcbin,
                         nullptr);

        if (!gst_element_link_many(source,
                                   volume,
                                   convert,
                                   resample,
                                   queue1,
                                   opusenc,
                                   rtp,
                                   queue2,
                                   capsfilter,
                                   webrtcbin,
                                   nullptr)) {
                nhlog::ui()->error("WebRTC: failed to link pipeline elements");
                end();
                return false;
        }
        return true;
}

bool
WebRTCSession::toggleMuteAudioSrc(bool &isMuted)
{
        if (state_ < State::INITIATED)
                return false;

        GstElement *srclevel = gst_bin_get_by_name(GST_BIN(pipe_), "srclevel");
        if (!srclevel)
                return false;

        gboolean muted;
        g_object_get(srclevel, "mute", &muted, nullptr);
        g_object_set(srclevel, "mute", !muted, nullptr);
        gst_object_unref(srclevel);
        isMuted = !muted;
        return true;
}

void
WebRTCSession::end()
{
        nhlog::ui()->debug("WebRTC: ending session");
        if (pipe_) {
                gst_element_set_state(pipe_, GST_STATE_NULL);
                gst_object_unref(pipe_);
                pipe_ = nullptr;
        }
        webrtc_ = nullptr;
        if (state_ != State::DISCONNECTED)
                emit stateChanged(State::DISCONNECTED);
}

void
WebRTCSession::refreshDevices()
{
        if (!initialised_)
                return;

        static GstDeviceMonitor *monitor = nullptr;
        if (!monitor) {
                monitor       = gst_device_monitor_new();
                GstCaps *caps = gst_caps_new_empty_simple("audio/x-raw");
                gst_device_monitor_add_filter(monitor, "Audio/Source", caps);
                gst_caps_unref(caps);
        }
        g_list_free_full(audioSources_, g_object_unref);
        audioSources_ = gst_device_monitor_get_devices(monitor);
}

std::vector<std::string>
WebRTCSession::getAudioSourceNames(const std::string &defaultDevice)
{
        if (!initialised_)
                return {};

        refreshDevices();
        std::vector<std::string> ret;
        ret.reserve(g_list_length(audioSources_));
        for (GList *l = audioSources_; l != nullptr; l = l->next) {
                gchar *name = gst_device_get_display_name(GST_DEVICE_CAST(l->data));
                ret.emplace_back(name);
                g_free(name);
                if (ret.back() == defaultDevice) {
                        // move default device to top of the list
                        std::swap(audioSources_->data, l->data);
                        std::swap(ret.front(), ret.back());
                }
        }
        return ret;
}
#else

bool
WebRTCSession::createOffer()
{
        return false;
}

bool
WebRTCSession::acceptOffer(const std::string &)
{
        return false;
}

bool
WebRTCSession::acceptAnswer(const std::string &)
{
        return false;
}

void
WebRTCSession::acceptICECandidates(const std::vector<mtx::events::msg::CallCandidates::Candidate> &)
{}

bool
WebRTCSession::startPipeline(int)
{
        return false;
}

bool
WebRTCSession::createPipeline(int)
{
        return false;
}

bool
WebRTCSession::toggleMuteAudioSrc(bool &)
{
        return false;
}

void
WebRTCSession::end()
{}

void
WebRTCSession::refreshDevices()
{}

std::vector<std::string>
WebRTCSession::getAudioSourceNames(const std::string &)
{
        return {};
}

#endif
