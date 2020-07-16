#include <cctype>

#include "WebRTCSession.h"
#include "Logging.h"

extern "C" {
#include "gst/gst.h"
#include "gst/sdp/sdp.h"

#define GST_USE_UNSTABLE_API
#include "gst/webrtc/webrtc.h"
}

namespace {
bool gisoffer;
std::string glocalsdp;
std::vector<mtx::events::msg::CallCandidates::Candidate> gcandidates;

gboolean newBusMessage(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, gpointer user_data);
GstWebRTCSessionDescription* parseSDP(const std::string &sdp, GstWebRTCSDPType type);
void generateOffer(GstElement *webrtc);
void setLocalDescription(GstPromise *promise, gpointer webrtc);
void addLocalICECandidate(GstElement *webrtc G_GNUC_UNUSED, guint mlineIndex, gchar *candidate, gpointer G_GNUC_UNUSED);
gboolean onICEGatheringCompletion(gpointer timerid);
void createAnswer(GstPromise *promise, gpointer webrtc);
void addDecodeBin(GstElement *webrtc G_GNUC_UNUSED, GstPad *newpad, GstElement *pipe);
void linkNewPad(GstElement *decodebin G_GNUC_UNUSED, GstPad *newpad, GstElement *pipe);
std::string::const_iterator  findName(const std::string &sdp, const std::string &name);
int getPayloadType(const std::string &sdp, const std::string &name);
}

bool
WebRTCSession::init(std::string *errorMessage)
{
  if (initialised_)
    return true;

  GError *error = nullptr;
  if (!gst_init_check(nullptr, nullptr, &error)) {
    std::string strError = std::string("Failed to initialise GStreamer: ");
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
  nhlog::ui()->info("Initialised " + gstVersion);

  // GStreamer Plugins:
  // Base:            audioconvert, audioresample, opus, playback, videoconvert, volume
  // Good:            autodetect, rtpmanager, vpx
  // Bad:             dtls, srtp, webrtc
  // libnice [GLib]:  nice
  initialised_ = true;
  std::string strError = gstVersion + ": Missing plugins: ";
  const gchar *needed[] = {"audioconvert", "audioresample", "autodetect", "dtls", "nice",
    "opus", "playback", "rtpmanager", "srtp", "videoconvert", "vpx", "volume", "webrtc", nullptr};
  GstRegistry *registry = gst_registry_get();
  for (guint i = 0; i < g_strv_length((gchar**)needed); i++) {
    GstPlugin *plugin = gst_registry_find_plugin(registry, needed[i]);
    if (!plugin) {
      strError += needed[i];
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
}

bool
WebRTCSession::createOffer()
{
  gisoffer = true;
  glocalsdp.clear();
  gcandidates.clear();
  return startPipeline(111); // a dynamic opus payload type
}

bool
WebRTCSession::acceptOffer(const std::string& sdp)
{
  nhlog::ui()->debug("Received offer:\n{}", sdp);
  gisoffer = false;
  glocalsdp.clear();
  gcandidates.clear();

  int opusPayloadType = getPayloadType(sdp, "opus"); 
  if (opusPayloadType == -1) {
    return false;
  }

  GstWebRTCSessionDescription *offer = parseSDP(sdp, GST_WEBRTC_SDP_TYPE_OFFER);
  if (!offer)
    return false;

  if (!startPipeline(opusPayloadType))
    return false;

  // set-remote-description first, then create-answer
  GstPromise *promise = gst_promise_new_with_change_func(createAnswer, webrtc_, nullptr);
  g_signal_emit_by_name(webrtc_, "set-remote-description", offer, promise);
  gst_webrtc_session_description_free(offer);
  return true;
}

bool
WebRTCSession::startPipeline(int opusPayloadType)
{
  if (isActive())
    return false;

  if (!createPipeline(opusPayloadType))
    return false;

  webrtc_ = gst_bin_get_by_name(GST_BIN(pipe_), "webrtcbin");

  if (!stunServer_.empty()) {
    nhlog::ui()->info("WebRTC: Setting STUN server: {}", stunServer_);
    g_object_set(webrtc_, "stun-server", stunServer_.c_str(), nullptr);
  }
  addTurnServers();

  // generate the offer when the pipeline goes to PLAYING
  if (gisoffer)
    g_signal_connect(webrtc_, "on-negotiation-needed", G_CALLBACK(generateOffer), nullptr);

  // on-ice-candidate is emitted when a local ICE candidate has been gathered
  g_signal_connect(webrtc_, "on-ice-candidate", G_CALLBACK(addLocalICECandidate), nullptr);

  // incoming streams trigger pad-added
  gst_element_set_state(pipe_, GST_STATE_READY);
  g_signal_connect(webrtc_, "pad-added", G_CALLBACK(addDecodeBin), pipe_);

  // webrtcbin lifetime is the same as that of the pipeline
  gst_object_unref(webrtc_);

  // start the pipeline
  GstStateChangeReturn ret = gst_element_set_state(pipe_, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    nhlog::ui()->error("WebRTC: unable to start pipeline");
    gst_object_unref(pipe_);
    pipe_ = nullptr;
    webrtc_ = nullptr;
    return false;
  }

  GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe_));
  gst_bus_add_watch(bus, newBusMessage, this);
  gst_object_unref(bus);
  emit pipelineChanged(true);
  return true;
}

#define RTP_CAPS_OPUS "application/x-rtp,media=audio,encoding-name=OPUS,payload="

bool
WebRTCSession::createPipeline(int opusPayloadType)
{
  std::string pipeline("webrtcbin bundle-policy=max-bundle name=webrtcbin "
      "autoaudiosrc ! volume name=srclevel ! audioconvert ! audioresample ! queue ! opusenc ! rtpopuspay ! "
      "queue ! " RTP_CAPS_OPUS  + std::to_string(opusPayloadType) + " ! webrtcbin.");

  webrtc_ = nullptr;
  GError *error = nullptr;
  pipe_ = gst_parse_launch(pipeline.c_str(), &error);
  if (error) {
    nhlog::ui()->error("WebRTC: Failed to parse pipeline: {}", error->message);
    g_error_free(error);
    if (pipe_) {
      gst_object_unref(pipe_);
      pipe_ = nullptr;
    }
    return false;
  }
  return true;
}

bool
WebRTCSession::acceptAnswer(const std::string &sdp)
{
  nhlog::ui()->debug("WebRTC: Received sdp:\n{}", sdp);
  if (!isActive())
    return false;

  GstWebRTCSessionDescription *answer = parseSDP(sdp, GST_WEBRTC_SDP_TYPE_ANSWER);
  if (!answer)
    return false;

  g_signal_emit_by_name(webrtc_, "set-remote-description", answer, nullptr);
  gst_webrtc_session_description_free(answer);
  return true;
}

void
WebRTCSession::acceptICECandidates(const std::vector<mtx::events::msg::CallCandidates::Candidate>& candidates)
{
  if (isActive()) {
    for (const auto& c : candidates)
      g_signal_emit_by_name(webrtc_, "add-ice-candidate", c.sdpMLineIndex, c.candidate.c_str());
  }
}

bool
WebRTCSession::toggleMuteAudioSrc(bool &isMuted)
{
  if (!isActive())
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
  if (pipe_) {
    gst_element_set_state(pipe_, GST_STATE_NULL);
    gst_object_unref(pipe_);
    pipe_ = nullptr;
  }
  webrtc_ = nullptr;
  emit pipelineChanged(false);
}

void
WebRTCSession::addTurnServers()
{
  if (!webrtc_)
    return;

  for (const auto &uri : turnServers_) {
    nhlog::ui()->info("WebRTC: Setting TURN server: {}", uri);
    gboolean udata;
    g_signal_emit_by_name(webrtc_, "add-turn-server", uri.c_str(), (gpointer)(&udata));
  }
}

namespace {

std::string::const_iterator findName(const std::string &sdp, const std::string &name)
{
  return std::search(sdp.cbegin(), sdp.cend(), name.cbegin(), name.cend(),
    [](unsigned char c1, unsigned char c2) {return std::tolower(c1) == std::tolower(c2);});
}

int getPayloadType(const std::string &sdp, const std::string &name)
{
  // eg a=rtpmap:111 opus/48000/2
  auto e = findName(sdp, name);
  if (e == sdp.cend()) {
    nhlog::ui()->error("WebRTC: remote offer - " + name + " attribute missing");
    return -1;
  }

  if (auto s = sdp.rfind(':', e - sdp.cbegin()); s == std::string::npos) {
    nhlog::ui()->error("WebRTC: remote offer - unable to determine " + name + " payload type");
    return -1;
  }
  else {
    ++s;
    try {
      return std::stoi(std::string(sdp, s, e - sdp.cbegin() - s));
    }
    catch(...) {
      nhlog::ui()->error("WebRTC: remote offer - unable to determine " + name + " payload type");
    }
  }
  return -1;
}

gboolean
newBusMessage(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, gpointer user_data)
{
  WebRTCSession *session = (WebRTCSession*)user_data;
  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
      session->end();
      break;
    case GST_MESSAGE_ERROR:
      GError *error;
      gchar *debug;
      gst_message_parse_error(msg, &error, &debug);
      nhlog::ui()->error("WebRTC: Error from element {}: {}", GST_OBJECT_NAME(msg->src), error->message);
      g_clear_error(&error);
      g_free(debug);
      session->end();
      break;
    default:
      break;
  }
  return TRUE;
}

GstWebRTCSessionDescription*
parseSDP(const std::string &sdp, GstWebRTCSDPType type)
{
  GstSDPMessage *msg;
  gst_sdp_message_new(&msg);
  if (gst_sdp_message_parse_buffer((guint8*)sdp.c_str(), sdp.size(), msg) == GST_SDP_OK) {
    return gst_webrtc_session_description_new(type, msg);
  }
  else {
    nhlog::ui()->error("WebRTC: Failed to parse remote session description");
    gst_object_unref(msg);
    return nullptr;
  }
}

void
generateOffer(GstElement *webrtc)
{
  // create-offer first, then set-local-description
  GstPromise *promise = gst_promise_new_with_change_func(setLocalDescription, webrtc, nullptr);
  g_signal_emit_by_name(webrtc, "create-offer", nullptr, promise);
}

void
setLocalDescription(GstPromise *promise, gpointer webrtc)
{
  const GstStructure *reply = gst_promise_get_reply(promise);
  gboolean isAnswer = gst_structure_id_has_field(reply, g_quark_from_string("answer"));
  GstWebRTCSessionDescription *gstsdp = nullptr;
  gst_structure_get(reply, isAnswer ? "answer" : "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &gstsdp, nullptr);
  gst_promise_unref(promise);
  g_signal_emit_by_name(webrtc, "set-local-description", gstsdp, nullptr);

  gchar *sdp = gst_sdp_message_as_text(gstsdp->sdp);
  glocalsdp = std::string(sdp);
  g_free(sdp);
  gst_webrtc_session_description_free(gstsdp);

  nhlog::ui()->debug("WebRTC: Local description set ({}):\n{}", isAnswer ? "answer" : "offer", glocalsdp);
}

void
addLocalICECandidate(GstElement *webrtc G_GNUC_UNUSED, guint mlineIndex, gchar *candidate, gpointer G_GNUC_UNUSED)
{
  gcandidates.push_back({"audio", (uint16_t)mlineIndex, candidate});

  // GStreamer v1.16: webrtcbin's notify::ice-gathering-state triggers GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE too early
  // fixed in v1.18
  // use a 100ms timeout in the meantime
  static guint timerid = 0;
  if (timerid)
    g_source_remove(timerid);

  timerid = g_timeout_add(100, onICEGatheringCompletion, &timerid);
}

gboolean
onICEGatheringCompletion(gpointer timerid)
{
  *(guint*)(timerid) = 0;
  if (gisoffer)
    emit WebRTCSession::instance().offerCreated(glocalsdp, gcandidates);
  else
    emit WebRTCSession::instance().answerCreated(glocalsdp, gcandidates);

  return FALSE;
}

void
createAnswer(GstPromise *promise, gpointer webrtc)
{
  // create-answer first, then set-local-description
  gst_promise_unref(promise);
  promise = gst_promise_new_with_change_func(setLocalDescription, webrtc, nullptr);
  g_signal_emit_by_name(webrtc, "create-answer", nullptr, promise);
}

void
addDecodeBin(GstElement *webrtc G_GNUC_UNUSED, GstPad *newpad, GstElement *pipe)
{
  if (GST_PAD_DIRECTION(newpad) != GST_PAD_SRC)
    return;

  GstElement *decodebin = gst_element_factory_make("decodebin", nullptr);
  g_signal_connect(decodebin, "pad-added", G_CALLBACK(linkNewPad), pipe);
  gst_bin_add(GST_BIN(pipe), decodebin);
  gst_element_sync_state_with_parent(decodebin);
  GstPad *sinkpad = gst_element_get_static_pad(decodebin, "sink");
  if (GST_PAD_LINK_FAILED(gst_pad_link(newpad, sinkpad)))
    nhlog::ui()->error("WebRTC: Unable to link new pad");
  gst_object_unref(sinkpad);
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
  GstElement *queue = gst_element_factory_make("queue", nullptr);

  if (g_str_has_prefix(name, "audio")) {
    GstElement *convert = gst_element_factory_make("audioconvert", nullptr);
    GstElement *resample = gst_element_factory_make("audioresample", nullptr);
    GstElement *sink = gst_element_factory_make("autoaudiosink", nullptr);
    gst_bin_add_many(GST_BIN(pipe), queue, convert, resample, sink, nullptr);
    gst_element_sync_state_with_parent(queue);
    gst_element_sync_state_with_parent(convert);
    gst_element_sync_state_with_parent(resample);
    gst_element_sync_state_with_parent(sink);
    gst_element_link_many(queue, convert, resample, sink, nullptr);
    queuepad = gst_element_get_static_pad(queue, "sink");
  }
  else if (g_str_has_prefix(name, "video")) {
    GstElement *convert = gst_element_factory_make("videoconvert", nullptr);
    GstElement *sink = gst_element_factory_make("autovideosink", nullptr);
    gst_bin_add_many(GST_BIN(pipe), queue, convert, sink, nullptr);
    gst_element_sync_state_with_parent(queue);
    gst_element_sync_state_with_parent(convert);
    gst_element_sync_state_with_parent(sink);
    gst_element_link_many(queue, convert, sink, nullptr);
    queuepad = gst_element_get_static_pad(queue, "sink");
  }

  if (queuepad) {
    if (GST_PAD_LINK_FAILED(gst_pad_link(newpad, queuepad)))
      nhlog::ui()->error("WebRTC: Unable to link new pad");
    gst_object_unref(queuepad);
  }
}

}
