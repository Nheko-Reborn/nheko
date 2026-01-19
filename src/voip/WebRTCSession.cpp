// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickItem>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string_view>
#include <thread>
#include <utility>

#include "CallDevices.h"
#include "ChatPage.h"
#include "Logging.h"
#include "UserSettingsPage.h"
#include "WebRTCSession.h"
#include "voip/ScreenCastPortal.h"

#ifdef GSTREAMER_AVAILABLE
#include "MainWindow.h"
extern "C"
{
#include "gst/gl/gstgldisplay.h"
#include "gst/gst.h"
#include "gst/sdp/sdp.h"

#define GST_USE_UNSTABLE_API
#include "gst/webrtc/webrtc.h"
}

#if !GST_CHECK_VERSION(1, 20, 0)
#define gst_element_request_pad_simple gst_element_get_request_pad
#endif

#endif

// https://github.com/vector-im/riot-web/issues/10173
#define STUN_SERVER "stun://turn.matrix.org:3478"

using webrtc::CallType;
using webrtc::ScreenShareType;
using webrtc::State;

WebRTCSession::WebRTCSession()
  : devices_(CallDevices::instance())
{
    // qmlRegisterUncreatableMetaObject(webrtc::staticMetaObject,
    //                                  "im.nheko",
    //                                  1,
    //                                  0,
    //                                  "CallType",
    //                                  QStringLiteral("Can't instantiate enum"));

    // qmlRegisterUncreatableMetaObject(webrtc::staticMetaObject,
    //                                  "im.nheko",
    //                                  1,
    //                                  0,
    //                                  "ScreenShareType",
    //                                  QStringLiteral("Can't instantiate enum"));

    // qmlRegisterUncreatableMetaObject(webrtc::staticMetaObject,
    //                                  "im.nheko",
    //                                  1,
    //                                  0,
    //                                  "WebRTCState",
    //                                  QStringLiteral("Can't instantiate enum"));

    connect(this, &WebRTCSession::stateChanged, this, &WebRTCSession::setState);
    init();
}

bool
WebRTCSession::init(std::string *errorMessage)
{
#ifdef GSTREAMER_AVAILABLE
    if (initialised_)
        return true;

    GError *error = nullptr;
    if (!gst_init_check(nullptr, nullptr, &error)) {
        std::string strError("WebRTC: failed to initialise GStreamer: ");
        if (error) {
            strError += error->message;
            g_error_free(error);
        }
        nhlog::ui()->error(strError);
        if (errorMessage)
            *errorMessage = strError;
        return false;
    }

    initialised_   = true;
    gchar *version = gst_version_string();
    nhlog::ui()->info("WebRTC: initialised {}", version);
    g_free(version);
    devices_.init();
    return true;
#else
    (void)errorMessage;
    return false;
#endif
}

#ifdef GSTREAMER_AVAILABLE
namespace {

std::string localsdp_;
std::vector<mtx::events::voip::CallCandidates::Candidate> localcandidates_;
bool haveAudioStream_     = false;
bool haveVideoStream_     = false;
GstPad *localPiPSinkPad_  = nullptr;
GstPad *remotePiPSinkPad_ = nullptr;

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
        session->setLastError(error->message);
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
    if (gst_sdp_message_parse_buffer((guint8 *)sdp.c_str(), static_cast<guint>(sdp.size()), msg) ==
        GST_SDP_OK) {
        return gst_webrtc_session_description_new(type, msg);
    } else {
        nhlog::ui()->error("WebRTC: failed to parse remote session description");
        gst_sdp_message_free(msg);
        return nullptr;
    }
}

void
setLocalDescription(GstPromise *promise, gpointer webrtc)
{
    const GstStructure *reply = gst_promise_get_reply(promise);
    gboolean isAnswer         = gst_structure_id_has_field(reply, g_quark_from_string("answer"));
    GstWebRTCSessionDescription *gstsdp = nullptr;
    gst_structure_get(
      reply, isAnswer ? "answer" : "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &gstsdp, nullptr);
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
    GstPromise *promise = gst_promise_new_with_change_func(setLocalDescription, webrtc, nullptr);
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

void
iceGatheringStateChanged(GstElement *webrtc,
                         GParamSpec *pspec G_GNUC_UNUSED,
                         gpointer user_data G_GNUC_UNUSED)
{
    GstWebRTCICEGatheringState newState;
    g_object_get(webrtc, "ice-gathering-state", &newState, nullptr);
    if (newState == GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE) {
        nhlog::ui()->debug("WebRTC: GstWebRTCICEGatheringState -> Complete");
        if (WebRTCSession::instance().isOffering()) {
            emit WebRTCSession::instance().offerCreated(localsdp_, localcandidates_);
            emit WebRTCSession::instance().stateChanged(State::OFFERSENT);
        } else {
            emit WebRTCSession::instance().answerCreated(localsdp_, localcandidates_);
            emit WebRTCSession::instance().stateChanged(State::ANSWERSENT);
        }
    }
}

void
addLocalICECandidate(GstElement *webrtc G_GNUC_UNUSED,
                     guint mlineIndex,
                     gchar *candidate,
                     gpointer G_GNUC_UNUSED)
{
    nhlog::ui()->debug("WebRTC: local candidate: (m-line:{}):{}", mlineIndex, candidate);
    localcandidates_.push_back({std::string() /*max-bundle*/, (uint16_t)mlineIndex, candidate});
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
        emit WebRTCSession::instance().stateChanged(State::CONNECTING);
        break;
    case GST_WEBRTC_ICE_CONNECTION_STATE_FAILED:
        nhlog::ui()->error("WebRTC: GstWebRTCICEConnectionState -> Failed");
        emit WebRTCSession::instance().stateChanged(State::ICEFAILED);
        break;
    default:
        break;
    }
}

// https://gitlab.freedesktop.org/gstreamer/gst-plugins-bad/-/issues/1164
struct KeyFrameRequestData
{
    GstElement *pipe      = nullptr;
    GstElement *decodebin = nullptr;
    gint packetsLost      = 0;
    guint timerid         = 0;
    std::string statsField;
} keyFrameRequestData_;

void
sendKeyFrameRequest()
{
    GstPad *sinkpad = gst_element_get_static_pad(keyFrameRequestData_.decodebin, "sink");
    if (!gst_pad_push_event(sinkpad,
                            gst_event_new_custom(GST_EVENT_CUSTOM_UPSTREAM,
                                                 gst_structure_new_empty("GstForceKeyUnit"))))
        nhlog::ui()->error("WebRTC: key frame request failed");
    else
        nhlog::ui()->debug("WebRTC: sent key frame request");

    gst_object_unref(sinkpad);
}

void
testPacketLoss_(GstPromise *promise, gpointer G_GNUC_UNUSED)
{
    const GstStructure *reply = gst_promise_get_reply(promise);
    gint packetsLost          = 0;
    GstStructure *rtpStats;
    if (!gst_structure_get(
          reply, keyFrameRequestData_.statsField.c_str(), GST_TYPE_STRUCTURE, &rtpStats, nullptr)) {
        nhlog::ui()->error("WebRTC: get-stats: no field: {}", keyFrameRequestData_.statsField);
        gst_promise_unref(promise);
        return;
    }
    gst_structure_get_int(rtpStats, "packets-lost", &packetsLost);
    gst_structure_free(rtpStats);
    gst_promise_unref(promise);
    if (packetsLost > keyFrameRequestData_.packetsLost) {
        nhlog::ui()->debug("WebRTC: inbound video lost packet count: {}", packetsLost);
        keyFrameRequestData_.packetsLost = packetsLost;
        sendKeyFrameRequest();
    }
}

gboolean
testPacketLoss(gpointer G_GNUC_UNUSED)
{
    if (keyFrameRequestData_.pipe) {
        GstElement *webrtc  = gst_bin_get_by_name(GST_BIN(keyFrameRequestData_.pipe), "webrtcbin");
        GstPromise *promise = gst_promise_new_with_change_func(testPacketLoss_, nullptr, nullptr);
        g_signal_emit_by_name(webrtc, "get-stats", nullptr, promise);
        gst_object_unref(webrtc);
        return TRUE;
    }
    return FALSE;
}

void
setWaitForKeyFrame(GstBin *decodebin G_GNUC_UNUSED, GstElement *element, gpointer G_GNUC_UNUSED)
{
    // Unconditionally enable keyframe wait and requesting keyframes, so that we do that for
    // every decode, not just vp8 decoding
    g_object_set(element, "wait-for-keyframe", TRUE, "request-keyframe", TRUE, nullptr);
}

GstElement *
newAudioSinkChain(GstElement *pipe)
{
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
    return queue;
}

GstElement *
newVideoSinkChain(GstElement *pipe)
{
    // use compositor for now; acceleration needs investigation
    GstElement *queue = gst_element_factory_make("queue", nullptr);

    auto graphicsApi       = MainWindow::instance()->graphicsApi();
    GstElement *compositor = gst_element_factory_make(
      graphicsApi == QSGRendererInterface::OpenGL ? "compositor" : "d3d11compositor", "compositor");
    g_object_set(compositor, "background", 1, nullptr);
    switch (graphicsApi) {
    case QSGRendererInterface::OpenGL: {
        GstElement *qmlglsink = gst_element_factory_make("qml6glsink", nullptr);
        GstElement *glsinkbin = gst_element_factory_make("glsinkbin", nullptr);

        g_object_set(qmlglsink, "widget", WebRTCSession::instance().getVideoItem(), nullptr);
        g_object_set(glsinkbin, "sink", qmlglsink, nullptr);
        gst_bin_add_many(GST_BIN(pipe), queue, compositor, glsinkbin, nullptr);
        gst_element_link_many(queue, compositor, glsinkbin, nullptr);

        gst_element_sync_state_with_parent(queue);
        gst_element_sync_state_with_parent(compositor);
        gst_element_sync_state_with_parent(glsinkbin);

        // to propagate context (hopefully)
        gst_element_set_state(qmlglsink, GST_STATE_READY);

        // Workaround: On wayland, when egl is used, gstreamer might terminate the display
        // connection. Prevent that by "leaking" a reference to the display. See
        // https://gitlab.freedesktop.org/gstreamer/gstreamer/-/merge_requests/3743
        if (QGuiApplication::platformName() == QStringLiteral("wayland")) {
            auto context = gst_element_get_context(qmlglsink, "gst.gl.GLDisplay");
            if (context) {
                GstGLDisplay *display;
                gst_context_get_gl_display(context, &display);
            }
        }
    } break;
    case QSGRendererInterface::Direct3D11: {
        GstElement *d3d11upload       = gst_element_factory_make("d3d11upload", nullptr);
        GstElement *d3d11colorconvert = gst_element_factory_make("d3d11colorconvert", nullptr);
        GstElement *qmld3d11sink      = gst_element_factory_make("qml6d3d11sink", nullptr);

        g_object_set(qmld3d11sink, "widget", WebRTCSession::instance().getVideoItem(), nullptr);
        gst_bin_add_many(
          GST_BIN(pipe), queue, d3d11upload, compositor, d3d11colorconvert, qmld3d11sink, nullptr);
        gst_element_link_many(
          queue, d3d11upload, compositor, d3d11colorconvert, qmld3d11sink, nullptr);

        gst_element_sync_state_with_parent(queue);
        gst_element_sync_state_with_parent(compositor);
        gst_element_sync_state_with_parent(d3d11upload);
        gst_element_sync_state_with_parent(d3d11colorconvert);

        // to propagate context (hopefully)
        gst_element_set_state(qmld3d11sink, GST_STATE_READY);
    } break;
    default:
        break;
    }

    return queue;
}

std::pair<int, int>
getResolution(GstPad *pad)
{
    std::pair<int, int> ret;
    GstCaps *caps         = gst_pad_get_current_caps(pad);
    const GstStructure *s = gst_caps_get_structure(caps, 0);
    gst_structure_get_int(s, "width", &ret.first);
    gst_structure_get_int(s, "height", &ret.second);
    gst_caps_unref(caps);
    return ret;
}

std::pair<int, int>
getResolution(GstElement *pipe, const gchar *elementName, const gchar *padName)
{
    GstElement *element = gst_bin_get_by_name(GST_BIN(pipe), elementName);
    GstPad *pad         = gst_element_get_static_pad(element, padName);
    auto ret            = getResolution(pad);
    gst_object_unref(pad);
    gst_object_unref(element);
    return ret;
}

std::pair<int, int>
getPiPDimensions(const std::pair<int, int> &resolution, int fullWidth, double scaleFactor)
{
    double pipWidth  = fullWidth * scaleFactor;
    double pipHeight = static_cast<double>(resolution.second) / resolution.first * pipWidth;
    return {
      static_cast<int>(std::ceil(pipWidth)),
      static_cast<int>(std::ceil(pipHeight)),
    };
}

void
addLocalPiP(GstElement *pipe, const std::pair<int, int> &videoCallSize)
{
    // embed localUser's camera into received video (CallType::VIDEO)
    // OR embed screen share into received video (CallType::SCREEN)
    GstElement *tee = gst_bin_get_by_name(GST_BIN(pipe), "videosrctee");
    if (!tee)
        return;

    GstElement *queue = gst_element_factory_make("queue", nullptr);
    gst_bin_add(GST_BIN(pipe), queue);
    gst_element_link(tee, queue);
    gst_element_sync_state_with_parent(queue);
    gst_object_unref(tee);

    GstElement *compositor = gst_bin_get_by_name(GST_BIN(pipe), "compositor");
    localPiPSinkPad_       = gst_element_request_pad_simple(compositor, "sink_%u");
    g_object_set(localPiPSinkPad_, "zorder", 2, nullptr);

    bool isVideo         = WebRTCSession::instance().callType() == CallType::VIDEO;
    const gchar *element = isVideo ? "camerafilter" : "screenshare";
    const gchar *pad     = isVideo ? "sink" : "src";
    auto resolution      = getResolution(pipe, element, pad);
    auto pipSize         = getPiPDimensions(resolution, videoCallSize.first, 0.25);
    nhlog::ui()->debug("WebRTC: local picture-in-picture: {}x{}", pipSize.first, pipSize.second);
    g_object_set(localPiPSinkPad_, "width", pipSize.first, "height", pipSize.second, nullptr);
    gint offset = videoCallSize.first / 80;
    g_object_set(localPiPSinkPad_, "xpos", offset, "ypos", offset, nullptr);

    GstPad *srcpad = gst_element_get_static_pad(queue, "src");
    if (GST_PAD_LINK_FAILED(gst_pad_link(srcpad, localPiPSinkPad_)))
        nhlog::ui()->error("WebRTC: failed to link local PiP elements");
    gst_object_unref(srcpad);
    gst_object_unref(compositor);
}

void
addRemotePiP(GstElement *pipe)
{
    // embed localUser's camera into screen image being shared
    if (remotePiPSinkPad_) {
        auto camRes   = getResolution(pipe, "camerafilter", "sink");
        auto shareRes = getResolution(pipe, "screenshare", "src");
        auto pipSize  = getPiPDimensions(camRes, shareRes.first, 0.2);
        nhlog::ui()->debug(
          "WebRTC: screen share picture-in-picture: {}x{}", pipSize.first, pipSize.second);

        gint offset = shareRes.first / 100;
        g_object_set(remotePiPSinkPad_, "zorder", 2, nullptr);
        g_object_set(remotePiPSinkPad_, "width", pipSize.first, "height", pipSize.second, nullptr);
        g_object_set(remotePiPSinkPad_,
                     "xpos",
                     shareRes.first - pipSize.first - offset,
                     "ypos",
                     shareRes.second - pipSize.second - offset,
                     nullptr);
    }
}

void
addLocalVideo(GstElement *pipe)
{
    GstElement *queue = newVideoSinkChain(pipe);
    GstElement *tee   = gst_bin_get_by_name(GST_BIN(pipe), "videosrctee");
    GstPad *srcpad    = gst_element_request_pad_simple(tee, "src_%u");
    GstPad *sinkpad   = gst_element_get_static_pad(queue, "sink");
    if (GST_PAD_LINK_FAILED(gst_pad_link(srcpad, sinkpad)))
        nhlog::ui()->error("WebRTC: failed to link videosrctee -> video sink chain");
    gst_object_unref(srcpad);
}

void
linkNewPad(GstElement *decodebin, GstPad *newpad, GstElement *pipe)
{
    GstPad *sinkpad               = gst_element_get_static_pad(decodebin, "sink");
    GstCaps *sinkcaps             = gst_pad_get_current_caps(sinkpad);
    const GstStructure *structure = gst_caps_get_structure(sinkcaps, 0);

    gchar *mediaType = nullptr;
    guint ssrc       = 0;
    gst_structure_get(
      structure, "media", G_TYPE_STRING, &mediaType, "ssrc", G_TYPE_UINT, &ssrc, nullptr);
    gst_caps_unref(sinkcaps);
    gst_object_unref(sinkpad);

    WebRTCSession *session = &WebRTCSession::instance();
    GstElement *queue      = nullptr;
    if (!std::strcmp(mediaType, "audio")) {
        nhlog::ui()->debug("WebRTC: received incoming audio stream");
        haveAudioStream_ = true;
        queue            = newAudioSinkChain(pipe);
    } else if (!std::strcmp(mediaType, "video")) {
        nhlog::ui()->debug("WebRTC: received incoming video stream");
        if (!session->getVideoItem()) {
            g_free(mediaType);
            nhlog::ui()->error("WebRTC: video call item not set");
            return;
        }
        haveVideoStream_ = true;
        keyFrameRequestData_.statsField =
          std::string("rtp-inbound-stream-stats_") + std::to_string(ssrc);
        queue              = newVideoSinkChain(pipe);
        auto videoCallSize = getResolution(newpad);
        nhlog::ui()->info(
          "WebRTC: incoming video resolution: {}x{}", videoCallSize.first, videoCallSize.second);
        addLocalPiP(pipe, videoCallSize);
    } else {
        g_free(mediaType);
        nhlog::ui()->error("WebRTC: unknown pad type: {}", GST_PAD_NAME(newpad));
        return;
    }

    GstPad *queuepad = gst_element_get_static_pad(queue, "sink");
    if (queuepad) {
        if (GST_PAD_LINK_FAILED(gst_pad_link(newpad, queuepad)))
            nhlog::ui()->error("WebRTC: unable to link new pad");
        else {
            if (session->callType() == CallType::VOICE ||
                (haveAudioStream_ && (haveVideoStream_ || session->isRemoteVideoRecvOnly()))) {
                emit session->stateChanged(State::CONNECTED);
                if (haveVideoStream_) {
                    keyFrameRequestData_.pipe      = pipe;
                    keyFrameRequestData_.decodebin = decodebin;
                    keyFrameRequestData_.timerid =
                      g_timeout_add_seconds(3, testPacketLoss, nullptr);
                }
                addRemotePiP(pipe);
                if (session->isRemoteVideoRecvOnly())
                    addLocalVideo(pipe);
            }
        }
        gst_object_unref(queuepad);
    }
    g_free(mediaType);

    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipe), GST_DEBUG_GRAPH_SHOW_VERBOSE, "newpad");
}

void
addDecodeBin(GstElement *webrtc G_GNUC_UNUSED, GstPad *newpad, GstElement *pipe)
{
    if (GST_PAD_DIRECTION(newpad) != GST_PAD_SRC)
        return;

    nhlog::ui()->debug("WebRTC: received incoming stream");
    GstElement *decodebin = gst_element_factory_make("decodebin", nullptr);
    // hardware decoding needs investigation; eg rendering fails if vaapi plugin installed
    g_object_set(decodebin, "force-sw-decoders", TRUE, nullptr);
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(linkNewPad), pipe);
    g_signal_connect(decodebin, "element-added", G_CALLBACK(setWaitForKeyFrame), nullptr);
    gst_bin_add(GST_BIN(pipe), decodebin);
    gst_element_sync_state_with_parent(decodebin);
    GstPad *sinkpad = gst_element_get_static_pad(decodebin, "sink");
    if (GST_PAD_LINK_FAILED(gst_pad_link(newpad, sinkpad)))
        nhlog::ui()->error("WebRTC: unable to link decodebin");
    gst_object_unref(sinkpad);
}

bool
contains(std::string_view str1, std::string_view str2)
{
    return std::search(str1.cbegin(),
                       str1.cend(),
                       str2.cbegin(),
                       str2.cend(),
                       [](unsigned char c1, unsigned char c2) {
                           return std::tolower(c1) == std::tolower(c2);
                       }) != str1.cend();
}

bool
getMediaAttributes(const GstSDPMessage *sdp,
                   const char *mediaType,
                   const char *encoding,
                   int &payloadType,
                   bool &recvOnly,
                   bool &sendOnly)
{
    payloadType = -1;
    recvOnly    = false;
    sendOnly    = false;
    for (guint mlineIndex = 0; mlineIndex < gst_sdp_message_medias_len(sdp); ++mlineIndex) {
        const GstSDPMedia *media = gst_sdp_message_get_media(sdp, mlineIndex);
        if (!std::strcmp(gst_sdp_media_get_media(media), mediaType)) {
            recvOnly            = gst_sdp_media_get_attribute_val(media, "recvonly") != nullptr;
            sendOnly            = gst_sdp_media_get_attribute_val(media, "sendonly") != nullptr;
            const gchar *rtpval = nullptr;
            for (guint n = 0; n == 0 || rtpval; ++n) {
                rtpval = gst_sdp_media_get_attribute_val_n(media, "rtpmap", n);
                if (rtpval && contains(rtpval, encoding)) {
                    payloadType = std::atoi(rtpval);
                    break;
                }
            }
            return true;
        }
    }
    return false;
}
}

bool
WebRTCSession::havePlugins(bool isVideo,
                           bool isScreenshare,
                           ScreenShareType screenShareType,
                           std::string *errorMessage)
{
    if (!initialised_ && !init(errorMessage))
        return false;

    static constexpr std::initializer_list<const char *> audio_elements = {"audioconvert",
                                                                           "audioresample",
                                                                           "autoaudiosink",
                                                                           "capsfilter",
                                                                           "decodebin",
                                                                           "opusenc",
                                                                           "queue",
                                                                           "rtpopuspay",
                                                                           "volume",
                                                                           "webrtcbin",
                                                                           "nicesrc",
                                                                           "nicesink"};

    static constexpr std::initializer_list<const char *> gl_video_elements = {
      "compositor",
      "glsinkbin",
      "glupload",
      "qml6glsink",
      "rtpvp8pay",
      "tee",
      "videoconvert",
      "videoscale",
      "vp8enc",
    };

    static constexpr std::initializer_list<const char *> d3d11_video_elements = {
      "compositor",
      "d3d11colorconvert",
      "d3d11videosink",
      "d3d11upload",
      "qml6d3d11sink",
      "rtpvp8pay",
      "tee",
      "videoconvert",
      "videoscale",
      "vp8enc",
    };

    std::string strError("Missing GStreamer elements: ");
    GstRegistry *registry = gst_registry_get();

    auto check_plugins = [&strError,
                          registry](const std::initializer_list<const char *> &elements) {
        bool havePlugins = true;
        for (const auto &element : elements) {
            GstPluginFeature *plugin =
              gst_registry_find_feature(registry, element, GST_TYPE_ELEMENT_FACTORY);
            if (!plugin) {
                havePlugins = false;
                strError += std::string(element) + " ";
                continue;
            }
            gst_object_unref(plugin);
        }

        return havePlugins;
    };

    haveVoicePlugins_ = check_plugins(audio_elements);

    // check both elements at once
    if (isVideo) {
        switch (MainWindow::instance()->graphicsApi()) {
        case QSGRendererInterface::OpenGL:
            haveVideoPlugins_ = check_plugins(gl_video_elements);
            break;
        case QSGRendererInterface::Direct3D11:
            haveVideoPlugins_ = check_plugins(d3d11_video_elements);
            break;
        default:
            haveVideoPlugins_ = false;
            break;
        }
    }

    bool haveScreensharePlugins = false;
    if (isScreenshare) {
        haveScreensharePlugins = check_plugins({"videorate"});
        if (haveScreensharePlugins) {
            if (QGuiApplication::platformName() == QStringLiteral("wayland")) {
                haveScreensharePlugins = check_plugins({"waylandsink"});
            } else if (QGuiApplication::platformName() == QStringLiteral("windows")) {
                haveScreensharePlugins = check_plugins({"d3d11videosink"});
            } else {
                haveScreensharePlugins = check_plugins({"ximagesink"});
            }
        }
        if (haveScreensharePlugins) {
            if (screenShareType == ScreenShareType::X11) {
                haveScreensharePlugins = check_plugins({"ximagesrc"});
            } else if (screenShareType == ScreenShareType::D3D11) {
                haveScreensharePlugins =
                  check_plugins({"d3d11screencapturesrc", "d3d11download", "d3d11convert"});
            } else {
                haveScreensharePlugins = check_plugins({"pipewiresrc"});
            }
        }
    }

    if (!haveVoicePlugins_ || (isVideo && !haveVideoPlugins_) ||
        (isScreenshare && !haveScreensharePlugins)) {
        nhlog::ui()->error(strError);
        if (errorMessage)
            *errorMessage = strError;
        return false;
    }

    if (isVideo || isScreenshare) {
        switch (MainWindow::instance()->graphicsApi()) {
        case QSGRendererInterface::OpenGL: {
            // load qmlglsink to register GStreamer's GstGLVideoItem QML type
            GstElement *qmlglsink = gst_element_factory_make("qml6glsink", nullptr);
            gst_object_unref(qmlglsink);
        } break;
        case QSGRendererInterface::Direct3D11: {
            GstElement *qmld3d11sink = gst_element_factory_make("qml6d3d11sink", nullptr);
            gst_object_unref(qmld3d11sink);
        } break;
        default:
            break;
        }
    }
    return true;
}

bool
WebRTCSession::createOffer(CallType callType,
                           ScreenShareType screenShareType,
                           uint32_t shareWindowId)
{
    lastError_.clear();
    clear();

    if (!initialised_ && !init(&lastError_))
        return false;

    isOffering_      = true;
    callType_        = callType;
    screenShareType_ = screenShareType;
    shareWindowId_   = shareWindowId;

    // opus and vp8 rtp payload types must be defined dynamically
    // therefore from the range [96-127]
    // see for example https://tools.ietf.org/html/rfc7587
    constexpr int opusPayloadType = 111;
    constexpr int vp8PayloadType  = 96;
    return startPipeline(opusPayloadType, vp8PayloadType);
}

bool
WebRTCSession::acceptOffer(const std::string &sdp)
{
    lastError_.clear();
    nhlog::ui()->debug("WebRTC: received offer:\n{}", sdp);
    if (state_ != State::DISCONNECTED) {
        lastError_ = "Session already in progress";
        return false;
    }

    clear();

    if (!initialised_ && !init(&lastError_))
        return false;
    GstWebRTCSessionDescription *offer = parseSDP(sdp, GST_WEBRTC_SDP_TYPE_OFFER);
    if (!offer)
        return false;

    int opusPayloadType;
    bool recvOnly;
    bool sendOnly;
    if (getMediaAttributes(offer->sdp, "audio", "opus", opusPayloadType, recvOnly, sendOnly)) {
        if (opusPayloadType == -1) {
            lastError_ = "Remote audio offer - no opus encoding";
            nhlog::ui()->error("WebRTC: remote audio offer - no opus encoding");
            gst_webrtc_session_description_free(offer);
            return false;
        }
    } else {
        lastError_ = "Remote offer - no audio media";
        nhlog::ui()->error("WebRTC: remote offer - no audio media");
        gst_webrtc_session_description_free(offer);
        return false;
    }

    int vp8PayloadType;
    bool isVideo = getMediaAttributes(
      offer->sdp, "video", "vp8", vp8PayloadType, isRemoteVideoRecvOnly_, isRemoteVideoSendOnly_);
    if (isVideo && vp8PayloadType == -1) {
        lastError_ = "Remote video offer - no vp8 encoding";
        nhlog::ui()->error("WebRTC: remote video offer - no vp8 encoding");
        gst_webrtc_session_description_free(offer);
        return false;
    }
    callType_ = isVideo ? CallType::VIDEO : CallType::VOICE;

    if (!startPipeline(opusPayloadType, vp8PayloadType)) {
        gst_webrtc_session_description_free(offer);
        return false;
    }

    // avoid a race that sometimes leaves the generated answer without media tracks (a=ssrc
    // lines)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // set-remote-description first, then create-answer
    GstPromise *promise = gst_promise_new_with_change_func(createAnswer, webrtc_, nullptr);
    g_signal_emit_by_name(webrtc_, "set-remote-description", offer, promise);
    gst_webrtc_session_description_free(offer);

    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipe_), GST_DEBUG_GRAPH_SHOW_VERBOSE, "accept");

    return true;
}

bool
WebRTCSession::acceptNegotiation(const std::string &sdp)
{
    nhlog::ui()->debug("WebRTC: received negotiation offer:\n{}", sdp);
    if (state_ == State::DISCONNECTED)
        return false;
    return false;
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

    if (callType_ != CallType::VOICE) {
        int unused;
        if (!getMediaAttributes(
              answer->sdp, "video", "vp8", unused, isRemoteVideoRecvOnly_, isRemoteVideoSendOnly_))
            isRemoteVideoRecvOnly_ = true;
    }

    g_signal_emit_by_name(webrtc_, "set-remote-description", answer, nullptr);
    gst_webrtc_session_description_free(answer);
    return true;
}

void
WebRTCSession::acceptICECandidates(
  const std::vector<mtx::events::voip::CallCandidates::Candidate> &candidates)
{
    if (state_ >= State::INITIATED) {
        for (const auto &c : candidates) {
            nhlog::ui()->debug(
              "WebRTC: remote candidate: (m-line:{}):{}", c.sdpMLineIndex, c.candidate);
            if (!c.candidate.empty()) {
                g_signal_emit_by_name(
                  webrtc_, "add-ice-candidate", c.sdpMLineIndex, c.candidate.c_str());
            }
        }
    }
}

bool
WebRTCSession::startPipeline(int opusPayloadType, int vp8PayloadType)
{
    if (state_ != State::DISCONNECTED)
        return false;

    emit stateChanged(State::INITIATING);

    if (!createPipeline(opusPayloadType, vp8PayloadType)) {
        end();
        return false;
    }

    webrtc_ = gst_bin_get_by_name(GST_BIN(pipe_), "webrtcbin");

    if (ChatPage::instance()->userSettings()->useStunServer()) {
        nhlog::ui()->info("WebRTC: setting STUN server: {}", STUN_SERVER);
        g_object_set(webrtc_, "stun-server", STUN_SERVER, nullptr);
    }

    for (const auto &uri : turnServers_) {
        nhlog::ui()->info("WebRTC: setting TURN server: {}", uri);
        gboolean udata;
        g_signal_emit_by_name(webrtc_, "add-turn-server", uri.c_str(), (gpointer)(&udata));
    }
    if (turnServers_.empty())
        nhlog::ui()->warn("WebRTC: no TURN server provided");

    // generate the offer when the pipeline goes to PLAYING
    if (isOffering_)
        g_signal_connect(webrtc_, "on-negotiation-needed", G_CALLBACK(::createOffer), nullptr);

    // on-ice-candidate is emitted when a local ICE candidate has been gathered
    g_signal_connect(webrtc_, "on-ice-candidate", G_CALLBACK(addLocalICECandidate), nullptr);

    // capture ICE failure
    g_signal_connect(
      webrtc_, "notify::ice-connection-state", G_CALLBACK(iceConnectionStateChanged), nullptr);

    // incoming streams trigger pad-added
    gst_element_set_state(pipe_, GST_STATE_READY);
    g_signal_connect(webrtc_, "pad-added", G_CALLBACK(addDecodeBin), pipe_);

    // capture ICE gathering completion
    g_signal_connect(
      webrtc_, "notify::ice-gathering-state", G_CALLBACK(iceGatheringStateChanged), nullptr);

    // webrtcbin lifetime is the same as that of the pipeline
    gst_object_unref(webrtc_);

    // start the pipeline
    GstStateChangeReturn ret = gst_element_set_state(pipe_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        lastError_ = "Unable to start pipeline";
        nhlog::ui()->error("WebRTC: unable to start pipeline");
        end();
        return false;
    }

    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipe_), GST_DEBUG_GRAPH_SHOW_VERBOSE, "start");

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe_));
    busWatchId_ = gst_bus_add_watch(bus, newBusMessage, this);
    gst_object_unref(bus);
    emit stateChanged(State::INITIATED);
    return true;
}

bool
WebRTCSession::createPipeline(int opusPayloadType, int vp8PayloadType)
{
    GstDevice *device = devices_.audioDevice();
    if (!device)
        return false;

    GstElement *source = gst_device_create_element(device, nullptr);
    gst_object_unref(device);
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
        lastError_ = "Failed to link audio pipeline elements";
        nhlog::ui()->error("WebRTC: failed to link audio pipeline elements");
        return false;
    }

    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipe_), GST_DEBUG_GRAPH_SHOW_VERBOSE, "created");

    return callType_ == CallType::VOICE || isRemoteVideoSendOnly_
             ? true
             : addVideoPipeline(vp8PayloadType);
}

bool
WebRTCSession::addVideoPipeline(int vp8PayloadType)
{
    // allow incoming video calls despite localUser having no webcam
    if (callType_ == CallType::VIDEO && !devices_.haveCamera())
        return !isOffering_;

    auto settings            = ChatPage::instance()->userSettings();
    GstElement *camerafilter = nullptr;
    GstElement *videoconvert = gst_element_factory_make("videoconvert", nullptr);
    GstElement *tee          = gst_element_factory_make("tee", "videosrctee");
    gst_bin_add_many(GST_BIN(pipe_), videoconvert, tee, nullptr);

    if (callType_ == CallType::VIDEO || (settings->screenSharePiP() && devices_.haveCamera())) {
        std::pair<int, int> resolution;
        std::pair<int, int> frameRate;
        GstDevice *device = devices_.videoDevice(resolution, frameRate);
        if (!device)
            return false;

        GstElement *camera = gst_device_create_element(device, nullptr);
        gst_object_unref(device);
        GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                            "width",
                                            G_TYPE_INT,
                                            resolution.first,
                                            "height",
                                            G_TYPE_INT,
                                            resolution.second,
                                            "framerate",
                                            GST_TYPE_FRACTION,
                                            frameRate.first,
                                            frameRate.second,
                                            nullptr);
        camerafilter  = gst_element_factory_make("capsfilter", "camerafilter");
        g_object_set(camerafilter, "caps", caps, nullptr);
        gst_caps_unref(caps);

        gst_bin_add_many(GST_BIN(pipe_), camera, camerafilter, nullptr);
        if (!gst_element_link_many(camera, videoconvert, camerafilter, nullptr)) {
            nhlog::ui()->error("WebRTC: failed to link camera elements");
            return false;
        }
        if (callType_ == CallType::VIDEO && !gst_element_link(camerafilter, tee)) {
            nhlog::ui()->error("WebRTC: failed to link camerafilter -> tee");
            return false;
        }
    }

    if (callType_ == CallType::SCREEN) {
        nhlog::ui()->debug("WebRTC: screen share frame rate: {} fps",
                           settings->screenShareFrameRate());
        nhlog::ui()->debug("WebRTC: screen share picture-in-picture: {}",
                           settings->screenSharePiP());
        nhlog::ui()->debug("WebRTC: screen share request remote camera: {}",
                           settings->screenShareRemoteVideo());
        nhlog::ui()->debug("WebRTC: screen share hide mouse cursor: {}",
                           settings->screenShareHideCursor());

        GstElement *screencastsrc = nullptr;

        if (screenShareType_ == ScreenShareType::X11) {
            GstElement *ximagesrc = gst_element_factory_make("ximagesrc", "screenshare");
            if (!ximagesrc) {
                nhlog::ui()->error("WebRTC: failed to create ximagesrc");
                return false;
            }
            g_object_set(ximagesrc, "use-damage", FALSE, nullptr);
            g_object_set(ximagesrc, "xid", shareWindowId_, nullptr);
            g_object_set(ximagesrc, "show-pointer", !settings->screenShareHideCursor(), nullptr);
            g_object_set(ximagesrc, "do-timestamp", (gboolean)1, nullptr);

            gst_bin_add(GST_BIN(pipe_), ximagesrc);
            screencastsrc = ximagesrc;
        } else if (screenShareType_ == ScreenShareType::D3D11) {
            GstElement *d3d11screensrc =
              gst_element_factory_make("d3d11screencapturesrc", "screenshare");
            if (!d3d11screensrc) {
                nhlog::ui()->error("WebRTC: failed to create d3d11screencapturesrc");
                gst_object_unref(pipe_);
                pipe_ = nullptr;
                return false;
            }
            g_object_set(
              d3d11screensrc, "window-handle", static_cast<guint64>(shareWindowId_), nullptr);
            g_object_set(
              d3d11screensrc, "show-cursor", !settings->screenShareHideCursor(), nullptr);
            g_object_set(d3d11screensrc, "do-timestamp", (gboolean)1, nullptr);
            gst_bin_add(GST_BIN(pipe_), d3d11screensrc);

            GstElement *d3d11convert = gst_element_factory_make("d3d11convert", nullptr);
            gst_bin_add(GST_BIN(pipe_), d3d11convert);
            if (!gst_element_link(d3d11screensrc, d3d11convert)) {
                nhlog::ui()->error("WebRTC: failed to link d3d11screencapturesrc -> d3d11convert");
                return false;
            }

            GstElement *d3d11download = gst_element_factory_make("d3d11download", nullptr);
            gst_bin_add(GST_BIN(pipe_), d3d11download);
            if (!gst_element_link(d3d11convert, d3d11download)) {
                nhlog::ui()->error("WebRTC: failed to link d3d11convert -> d3d11download");
                return false;
            }
            screencastsrc = d3d11download;
        } else {
            ScreenCastPortal &sc_portal = ScreenCastPortal::instance();
            GstElement *pipewiresrc     = gst_element_factory_make("pipewiresrc", "screenshare");
            if (!pipewiresrc) {
                nhlog::ui()->error("WebRTC: failed to create pipewiresrc");
                gst_object_unref(pipe_);
                pipe_ = nullptr;
                return false;
            }

            const ScreenCastPortal::Stream *stream = sc_portal.getStream();
            if (stream == nullptr) {
                nhlog::ui()->error("xdg-desktop-portal stream not started");
                gst_object_unref(pipe_);
                pipe_ = nullptr;
                return false;
            }
            g_object_set(pipewiresrc, "fd", (gint)stream->fd.fileDescriptor(), nullptr);
            std::string path = std::to_string(stream->nodeId);
            g_object_set(pipewiresrc, "path", path.c_str(), nullptr);
            g_object_set(pipewiresrc, "do-timestamp", (gboolean)1, nullptr);
            gst_bin_add(GST_BIN(pipe_), pipewiresrc);
            GstElement *videorate = gst_element_factory_make("videorate", nullptr);
            gst_bin_add(GST_BIN(pipe_), videorate);
            if (!gst_element_link(pipewiresrc, videorate)) {
                nhlog::ui()->error("WebRTC: failed to link pipewiresrc -> videorate");
                return false;
            }
            screencastsrc = videorate;
        }

        GstCaps *caps          = gst_caps_new_simple("video/x-raw",
                                            "format",
                                            G_TYPE_STRING,
                                            "I420", // For vp8enc
                                            "framerate",
                                            GST_TYPE_FRACTION,
                                            settings->screenShareFrameRate(),
                                            1,
                                            nullptr);
        GstElement *capsfilter = gst_element_factory_make("capsfilter", nullptr);
        g_object_set(capsfilter, "caps", caps, nullptr);
        gst_caps_unref(caps);
        gst_bin_add(GST_BIN(pipe_), capsfilter);

        if (settings->screenSharePiP() && devices_.haveCamera()) {
            GstElement *compositor = gst_element_factory_make("compositor", nullptr);
            g_object_set(compositor, "background", 1, nullptr);
            gst_bin_add(GST_BIN(pipe_), compositor);
            if (!gst_element_link_many(screencastsrc, compositor, capsfilter, tee, nullptr)) {
                nhlog::ui()->error("WebRTC: failed to link screen share elements");
                return false;
            }

            GstPad *srcpad    = gst_element_get_static_pad(camerafilter, "src");
            remotePiPSinkPad_ = gst_element_request_pad_simple(compositor, "sink_%u");
            if (GST_PAD_LINK_FAILED(gst_pad_link(srcpad, remotePiPSinkPad_))) {
                nhlog::ui()->error("WebRTC: failed to link camerafilter -> compositor");
                gst_object_unref(srcpad);
                return false;
            }
            gst_object_unref(srcpad);
        } else if (!gst_element_link_many(screencastsrc, videoconvert, capsfilter, tee, nullptr)) {
            lastError_ = "Failed to link screen share elements";
            nhlog::ui()->error("WebRTC: failed to link screen share elements");
            return false;
        }
    }

    GstElement *queue  = gst_element_factory_make("queue", nullptr);
    GstElement *vp8enc = gst_element_factory_make("vp8enc", nullptr);
    g_object_set(vp8enc, "deadline", 1, nullptr);
    g_object_set(vp8enc, "error-resilient", 1, nullptr);
    GstElement *rtpvp8pay     = gst_element_factory_make("rtpvp8pay", nullptr);
    GstElement *rtpqueue      = gst_element_factory_make("queue", nullptr);
    GstElement *rtpcapsfilter = gst_element_factory_make("capsfilter", nullptr);
    GstCaps *rtpcaps          = gst_caps_new_simple("application/x-rtp",
                                           "media",
                                           G_TYPE_STRING,
                                           "video",
                                           "encoding-name",
                                           G_TYPE_STRING,
                                           "VP8",
                                           "payload",
                                           G_TYPE_INT,
                                           vp8PayloadType,
                                           nullptr);
    g_object_set(rtpcapsfilter, "caps", rtpcaps, nullptr);
    gst_caps_unref(rtpcaps);

    gst_bin_add_many(GST_BIN(pipe_), queue, vp8enc, rtpvp8pay, rtpqueue, rtpcapsfilter, nullptr);

    GstElement *webrtcbin = gst_bin_get_by_name(GST_BIN(pipe_), "webrtcbin");
    if (!gst_element_link_many(
          tee, queue, vp8enc, rtpvp8pay, rtpqueue, rtpcapsfilter, webrtcbin, nullptr)) {
        nhlog::ui()->error("WebRTC: failed to link rtp video elements");
        gst_object_unref(webrtcbin);
        return false;
    }

    if (callType_ == CallType::SCREEN &&
        !ChatPage::instance()->userSettings()->screenShareRemoteVideo()) {
        GArray *transceivers;
        g_signal_emit_by_name(webrtcbin, "get-transceivers", &transceivers);
        GstWebRTCRTPTransceiver *transceiver =
          g_array_index(transceivers, GstWebRTCRTPTransceiver *, 1);
        g_object_set(
          transceiver, "direction", GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY, nullptr);
        g_array_unref(transceivers);
    }

    gst_object_unref(webrtcbin);

    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipe_), GST_DEBUG_GRAPH_SHOW_VERBOSE, "addvideo");
    return true;
}

bool
WebRTCSession::haveLocalPiP() const
{
    if (state_ >= State::INITIATED) {
        if (callType_ == CallType::VOICE || isRemoteVideoRecvOnly_)
            return false;
        else if (callType_ == CallType::SCREEN)
            return true;
        else {
            GstElement *tee = gst_bin_get_by_name(GST_BIN(pipe_), "videosrctee");
            if (tee) {
                gst_object_unref(tee);
                return true;
            }
        }
    }
    return false;
}

bool
WebRTCSession::isMicMuted() const
{
    if (state_ < State::INITIATED)
        return false;

    GstElement *srclevel = gst_bin_get_by_name(GST_BIN(pipe_), "srclevel");
    gboolean muted;
    g_object_get(srclevel, "mute", &muted, nullptr);
    gst_object_unref(srclevel);
    return muted;
}

bool
WebRTCSession::toggleMicMute()
{
    if (state_ < State::INITIATED)
        return false;

    GstElement *srclevel = gst_bin_get_by_name(GST_BIN(pipe_), "srclevel");
    gboolean muted;
    g_object_get(srclevel, "mute", &muted, nullptr);
    g_object_set(srclevel, "mute", !muted, nullptr);
    gst_object_unref(srclevel);

    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipe_), GST_DEBUG_GRAPH_SHOW_VERBOSE, "togglemute");

    return !muted;
}

void
WebRTCSession::toggleLocalPiP()
{
    if (localPiPSinkPad_) {
        guint zorder;
        g_object_get(localPiPSinkPad_, "zorder", &zorder, nullptr);
        g_object_set(localPiPSinkPad_, "zorder", zorder ? 0 : 2, nullptr);
    }
}

void
WebRTCSession::clear()
{
    callType_              = webrtc::CallType::VOICE;
    isOffering_            = false;
    isRemoteVideoRecvOnly_ = false;
    isRemoteVideoSendOnly_ = false;
    pipe_                  = nullptr;
    webrtc_                = nullptr;
    busWatchId_            = 0;
    shareWindowId_         = 0;
    haveAudioStream_       = false;
    haveVideoStream_       = false;
    localPiPSinkPad_       = nullptr;
    remotePiPSinkPad_      = nullptr;
    localsdp_.clear();
    localcandidates_.clear();
}

void
WebRTCSession::end()
{
    nhlog::ui()->debug("WebRTC: ending session");
    keyFrameRequestData_ = KeyFrameRequestData{};
    if (pipe_) {
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipe_), GST_DEBUG_GRAPH_SHOW_VERBOSE, "end");

        gst_element_set_state(pipe_, GST_STATE_NULL);
        gst_object_unref(pipe_);
        pipe_ = nullptr;
        if (busWatchId_) {
            g_source_remove(busWatchId_);
            busWatchId_ = 0;
        }
    }

    clear();
    if (state_ != State::DISCONNECTED)
        emit stateChanged(State::DISCONNECTED);
}

#else

bool
WebRTCSession::havePlugins(bool, bool, ScreenShareType, std::string *)
{
    return false;
}

bool
WebRTCSession::haveLocalPiP() const
{
    return false;
}

// clang-format off
// clang-format < 12 is buggy on this
bool
WebRTCSession::createOffer(webrtc::CallType,
                           ScreenShareType screenShareType,
                           uint32_t)
{
    (void)screenShareType;
    return false;
}
// clang-format on

bool
WebRTCSession::acceptNegotiation(const std::string &)
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
WebRTCSession::acceptICECandidates(
  const std::vector<mtx::events::voip::CallCandidates::Candidate> &)
{
}

bool
WebRTCSession::isMicMuted() const
{
    return false;
}

bool
WebRTCSession::toggleMicMute()
{
    return false;
}

void
WebRTCSession::toggleLocalPiP()
{
}

void
WebRTCSession::end()
{
}

#endif

#include "moc_WebRTCSession.cpp"
