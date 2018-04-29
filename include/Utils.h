#pragma once

#include "Cache.h"
#include "RoomInfoListItem.h"
#include "timeline/widgets/AudioItem.h"
#include "timeline/widgets/FileItem.h"
#include "timeline/widgets/ImageItem.h"
#include "timeline/widgets/VideoItem.h"

#include <QDateTime>
#include <mtx/events/collections.hpp>

namespace utils {

using TimelineEvent = mtx::events::collections::TimelineEvents;

//! Human friendly timestamp representation.
QString
descriptiveTime(const QDateTime &then);

//! Generate a message description from the event to be displayed
//! in the RoomList.
DescInfo
getMessageDescription(const TimelineEvent &event, const QString &localUser, const QString &room_id);

//! Get the first character of a string, taking into account that
//! surrogate pairs might be in use.
QString
firstChar(const QString &input);

//! Get a human readable file size with the appropriate units attached.
QString
humanReadableFileSize(uint64_t bytes);

//! Match widgets/events with a description message.
template<class T>
QString
messageDescription(const QString &username = "", const QString &body = "")
{
        using Audio   = mtx::events::RoomEvent<mtx::events::msg::Audio>;
        using Emote   = mtx::events::RoomEvent<mtx::events::msg::Emote>;
        using File    = mtx::events::RoomEvent<mtx::events::msg::File>;
        using Image   = mtx::events::RoomEvent<mtx::events::msg::Image>;
        using Notice  = mtx::events::RoomEvent<mtx::events::msg::Notice>;
        using Sticker = mtx::events::Sticker;
        using Text    = mtx::events::RoomEvent<mtx::events::msg::Text>;
        using Video   = mtx::events::RoomEvent<mtx::events::msg::Video>;

        if (std::is_same<T, AudioItem>::value || std::is_same<T, Audio>::value)
                return QString("sent an audio clip");
        else if (std::is_same<T, ImageItem>::value || std::is_same<T, Image>::value)
                return QString("sent an image");
        else if (std::is_same<T, FileItem>::value || std::is_same<T, File>::value)
                return QString("sent a file");
        else if (std::is_same<T, VideoItem>::value || std::is_same<T, Video>::value)
                return QString("sent a video clip");
        else if (std::is_same<T, StickerItem>::value || std::is_same<T, Sticker>::value)
                return QString("sent a sticker");
        else if (std::is_same<T, Notice>::value)
                return QString("sent a notification");
        else if (std::is_same<T, Text>::value)
                return QString(": %1").arg(body);
        else if (std::is_same<T, Emote>::value)
                return QString("* %1 %2").arg(username).arg(body);
}

template<class T, class Event>
DescInfo
createDescriptionInfo(const Event &event, const QString &localUser, const QString &room_id)
{
        using Text  = mtx::events::RoomEvent<mtx::events::msg::Text>;
        using Emote = mtx::events::RoomEvent<mtx::events::msg::Emote>;

        const auto msg    = mpark::get<T>(event);
        const auto sender = QString::fromStdString(msg.sender);

        const auto username = Cache::displayName(room_id, sender);
        const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);

        bool isText  = std::is_same<T, Text>::value;
        bool isEmote = std::is_same<T, Emote>::value;

        return DescInfo{
          isEmote ? "" : (sender == localUser ? "You" : username),
          sender,
          (isText || isEmote)
            ? messageDescription<T>(username, QString::fromStdString(msg.content.body).trimmed())
            : QString(" %1").arg(messageDescription<T>()),
          utils::descriptiveTime(ts),
          ts};
}

//! Scale down an image to fit to the given width & height limitations.
template<class ImageType>
ImageType
scaleDown(uint64_t max_width, uint64_t max_height, const ImageType &source)
{
        if (source.isNull())
                return QPixmap();

        auto width_ratio  = (double)max_width / (double)source.width();
        auto height_ratio = (double)max_height / (double)source.height();

        auto min_aspect_ratio = std::min(width_ratio, height_ratio);

        int final_width  = 0;
        int final_height = 0;

        if (min_aspect_ratio > 1) {
                final_width  = source.width();
                final_height = source.height();
        } else {
                final_width  = source.width() * min_aspect_ratio;
                final_height = source.height() * min_aspect_ratio;
        }

        return source.scaled(
          final_width, final_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

//! Delete items in a container based on a predicate.
template<typename ContainerT, typename PredicateT>
void
erase_if(ContainerT &items, const PredicateT &predicate)
{
        for (auto it = items.begin(); it != items.end();) {
                if (predicate(*it))
                        it = items.erase(it);
                else
                        ++it;
        }
}

//! Calculate the Levenshtein distance between two strings with character skipping.
int
levenshtein_distance(const std::string &s1, const std::string &s2);
}
