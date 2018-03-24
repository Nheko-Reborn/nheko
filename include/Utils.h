#pragma once

#include "RoomInfoListItem.h"

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
getMessageDescription(const TimelineEvent &event, const QString &localUser);

//! Get the first character of a string, taking into account that
//! surrogate pairs might be in use.
QString
firstChar(const QString &input);

//! Get a human readable file size with the appropriate units attached.
QString
humanReadableFileSize(uint64_t bytes);

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

//! Calculate the Levenshtein distance between two strings with character skipping.
int
levenshtein_distance(const std::string &s1, const std::string &s2);
}
