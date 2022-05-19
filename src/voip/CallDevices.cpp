// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstring>
#include <optional>
#include <string_view>

#include "CallDevices.h"
#include "Logging.h"
#include "UserSettingsPage.h"

#ifdef GSTREAMER_AVAILABLE
extern "C"
{
#include "gst/gst.h"
}
#endif

CallDevices::CallDevices()
  : QObject()
{}

#ifdef GSTREAMER_AVAILABLE
namespace {

struct AudioSource
{
    std::string name;
    GstDevice *device;
};

struct VideoSource
{
    struct Caps
    {
        std::string resolution;
        std::vector<std::string> frameRates;
    };
    std::string name;
    GstDevice *device;
    std::vector<Caps> caps;
};

std::vector<AudioSource> audioSources_;
std::vector<VideoSource> videoSources_;

using FrameRate = std::pair<int, int>;
std::optional<FrameRate>
getFrameRate(const GValue *value)
{
    if (GST_VALUE_HOLDS_FRACTION(value)) {
        gint num = gst_value_get_fraction_numerator(value);
        gint den = gst_value_get_fraction_denominator(value);
        return FrameRate{num, den};
    }
    return std::nullopt;
}

void
addFrameRate(std::vector<std::string> &rates, const FrameRate &rate)
{
    constexpr double minimumFrameRate = 15.0;
    if (static_cast<double>(rate.first) / rate.second >= minimumFrameRate)
        rates.push_back(std::to_string(rate.first) + "/" + std::to_string(rate.second));
}

void
setDefaultDevice(bool isVideo)
{
    auto settings = UserSettings::instance();
    if (isVideo && settings->camera().isEmpty()) {
        const VideoSource &camera = videoSources_.front();
        settings->setCamera(QString::fromStdString(camera.name));
        settings->setCameraResolution(QString::fromStdString(camera.caps.front().resolution));
        settings->setCameraFrameRate(
          QString::fromStdString(camera.caps.front().frameRates.front()));
    } else if (!isVideo && settings->microphone().isEmpty()) {
        settings->setMicrophone(QString::fromStdString(audioSources_.front().name));
    }
}

void
addDevice(GstDevice *device)
{
    if (!device)
        return;

    gchar *name  = gst_device_get_display_name(device);
    gchar *type  = gst_device_get_device_class(device);
    bool isVideo = !std::strncmp(type, "Video", 5);
    g_free(type);
    nhlog::ui()->debug("WebRTC: {} device added: {}", isVideo ? "video" : "audio", name);
    if (!isVideo) {
        audioSources_.push_back({name, device});
        g_free(name);
        setDefaultDevice(false);
        return;
    }

    GstCaps *gstcaps = gst_device_get_caps(device);
    if (!gstcaps) {
        nhlog::ui()->debug("WebRTC: unable to get caps for {}", name);
        g_free(name);
        return;
    }

    VideoSource source{name, device, {}};
    g_free(name);
    guint nCaps = gst_caps_get_size(gstcaps);
    for (guint i = 0; i < nCaps; ++i) {
        GstStructure *structure  = gst_caps_get_structure(gstcaps, i);
        const gchar *struct_name = gst_structure_get_name(structure);
        if (!std::strcmp(struct_name, "video/x-raw")) {
            gint widthpx, heightpx;
            if (gst_structure_get(structure,
                                  "width",
                                  G_TYPE_INT,
                                  &widthpx,
                                  "height",
                                  G_TYPE_INT,
                                  &heightpx,
                                  nullptr)) {
                VideoSource::Caps caps;
                caps.resolution     = std::to_string(widthpx) + "x" + std::to_string(heightpx);
                const GValue *value = gst_structure_get_value(structure, "framerate");
                if (auto fr = getFrameRate(value); fr)
                    addFrameRate(caps.frameRates, *fr);
                else if (GST_VALUE_HOLDS_FRACTION_RANGE(value)) {
                    addFrameRate(caps.frameRates,
                                 *getFrameRate(gst_value_get_fraction_range_min(value)));
                    addFrameRate(caps.frameRates,
                                 *getFrameRate(gst_value_get_fraction_range_max(value)));
                } else if (GST_VALUE_HOLDS_LIST(value)) {
                    guint nRates = gst_value_list_get_size(value);
                    for (guint j = 0; j < nRates; ++j) {
                        const GValue *rate = gst_value_list_get_value(value, j);
                        if (auto frate = getFrameRate(rate); frate)
                            addFrameRate(caps.frameRates, *frate);
                    }
                }
                if (!caps.frameRates.empty())
                    source.caps.push_back(std::move(caps));
            }
        }
    }
    gst_caps_unref(gstcaps);
    videoSources_.push_back(std::move(source));
    setDefaultDevice(true);
}

template<typename T>
bool
removeDevice(T &sources, GstDevice *device, bool changed)
{
    if (auto it = std::find_if(
          sources.begin(), sources.end(), [device](const auto &s) { return s.device == device; });
        it != sources.end()) {
        nhlog::ui()->debug("WebRTC: device {}: {}", (changed ? "changed" : "removed"), it->name);
        gst_object_unref(device);
        sources.erase(it);
        return true;
    }
    return false;
}

void
removeDevice(GstDevice *device, bool changed)
{
    if (device) {
        if (removeDevice(audioSources_, device, changed) ||
            removeDevice(videoSources_, device, changed))
            return;
    }
}

gboolean
newBusMessage(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, gpointer user_data G_GNUC_UNUSED)
{
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_DEVICE_ADDED: {
        GstDevice *device;
        gst_message_parse_device_added(msg, &device);
        addDevice(device);
        emit CallDevices::instance().devicesChanged();
        break;
    }
    case GST_MESSAGE_DEVICE_REMOVED: {
        GstDevice *device;
        gst_message_parse_device_removed(msg, &device);
        removeDevice(device, false);
        emit CallDevices::instance().devicesChanged();
        break;
    }
    case GST_MESSAGE_DEVICE_CHANGED: {
        GstDevice *device;
        GstDevice *oldDevice;
        gst_message_parse_device_changed(msg, &device, &oldDevice);
        removeDevice(oldDevice, true);
        addDevice(device);
        break;
    }
    default:
        break;
    }
    return TRUE;
}

template<typename T>
std::vector<std::string>
deviceNames(T &sources, const std::string &defaultDevice)
{
    std::vector<std::string> ret;
    ret.reserve(sources.size());
    for (const auto &s : sources)
        ret.push_back(s.name);

    // move default device to top of the list
    if (auto it = std::find(ret.begin(), ret.end(), defaultDevice); it != ret.end())
        std::swap(ret.front(), *it);

    return ret;
}

std::optional<VideoSource>
getVideoSource(const std::string &cameraName)
{
    if (auto it = std::find_if(videoSources_.cbegin(),
                               videoSources_.cend(),
                               [&cameraName](const auto &s) { return s.name == cameraName; });
        it != videoSources_.cend()) {
        return *it;
    }
    return std::nullopt;
}

std::pair<int, int>
tokenise(std::string_view str, char delim)
{
    std::pair<int, int> ret;
    ret.first  = std::atoi(str.data());
    auto pos   = str.find_first_of(delim);
    ret.second = std::atoi(str.data() + pos + 1);
    return ret;
}
}

void
CallDevices::init()
{
    static GstDeviceMonitor *monitor = nullptr;
    if (!monitor) {
        monitor       = gst_device_monitor_new();
        GstCaps *caps = gst_caps_new_empty_simple("audio/x-raw");
        gst_device_monitor_add_filter(monitor, "Audio/Source", caps);
        gst_device_monitor_add_filter(monitor, "Audio/Duplex", caps);
        gst_caps_unref(caps);
        caps = gst_caps_new_empty_simple("video/x-raw");
        gst_device_monitor_add_filter(monitor, "Video/Source", caps);
        gst_device_monitor_add_filter(monitor, "Video/Duplex", caps);
        gst_caps_unref(caps);

        GstBus *bus = gst_device_monitor_get_bus(monitor);
        gst_bus_add_watch(bus, newBusMessage, nullptr);
        gst_object_unref(bus);
        if (!gst_device_monitor_start(monitor)) {
            nhlog::ui()->error("WebRTC: failed to start device monitor");
            return;
        }
    }
}

bool
CallDevices::haveMic() const
{
    return !audioSources_.empty();
}

bool
CallDevices::haveCamera() const
{
    return !videoSources_.empty();
}

std::vector<std::string>
CallDevices::names(bool isVideo, const std::string &defaultDevice) const
{
    return isVideo ? deviceNames(videoSources_, defaultDevice)
                   : deviceNames(audioSources_, defaultDevice);
}

std::vector<std::string>
CallDevices::resolutions(const std::string &cameraName) const
{
    std::vector<std::string> ret;
    if (auto s = getVideoSource(cameraName); s) {
        ret.reserve(s->caps.size());
        for (const auto &c : s->caps)
            ret.push_back(c.resolution);
    }
    return ret;
}

std::vector<std::string>
CallDevices::frameRates(const std::string &cameraName, const std::string &resolution) const
{
    if (auto s = getVideoSource(cameraName); s) {
        if (auto it = std::find_if(s->caps.cbegin(),
                                   s->caps.cend(),
                                   [&](const auto &c) { return c.resolution == resolution; });
            it != s->caps.cend())
            return it->frameRates;
    }
    return {};
}

GstDevice *
CallDevices::audioDevice() const
{
    std::string name = UserSettings::instance()->microphone().toStdString();
    if (auto it = std::find_if(audioSources_.cbegin(),
                               audioSources_.cend(),
                               [&name](const auto &s) { return s.name == name; });
        it != audioSources_.cend()) {
        nhlog::ui()->debug("WebRTC: microphone: {}", name);
        return it->device;
    } else {
        nhlog::ui()->error("WebRTC: unknown microphone: {}", name);
        return nullptr;
    }
}

GstDevice *
CallDevices::videoDevice(std::pair<int, int> &resolution, std::pair<int, int> &frameRate) const
{
    auto settings    = UserSettings::instance();
    std::string name = settings->camera().toStdString();
    if (auto s = getVideoSource(name); s) {
        nhlog::ui()->debug("WebRTC: camera: {}", name);
        resolution = tokenise(settings->cameraResolution().toStdString(), 'x');
        frameRate  = tokenise(settings->cameraFrameRate().toStdString(), '/');
        nhlog::ui()->debug("WebRTC: camera resolution: {}x{}", resolution.first, resolution.second);
        nhlog::ui()->debug("WebRTC: camera frame rate: {}/{}", frameRate.first, frameRate.second);
        return s->device;
    } else {
        nhlog::ui()->error("WebRTC: unknown camera: {}", name);
        return nullptr;
    }
}

#else

bool
CallDevices::haveMic() const
{
    return false;
}

bool
CallDevices::haveCamera() const
{
    return false;
}

std::vector<std::string>
CallDevices::names(bool, const std::string &) const
{
    return {};
}

std::vector<std::string>
CallDevices::resolutions(const std::string &) const
{
    return {};
}

std::vector<std::string>
CallDevices::frameRates(const std::string &, const std::string &) const
{
    return {};
}

#endif
