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
}
