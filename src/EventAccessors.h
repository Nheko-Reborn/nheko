#pragma once

#include <string>

#include <QDateTime>
#include <QString>

#include <mtx/events/collections.hpp>

namespace mtx::accessors {
std::string
event_id(const mtx::events::collections::TimelineEvents &event);

std::string
room_id(const mtx::events::collections::TimelineEvents &event);

std::string
sender(const mtx::events::collections::TimelineEvents &event);

QDateTime
origin_server_ts(const mtx::events::collections::TimelineEvents &event);

std::string
filename(const mtx::events::collections::TimelineEvents &event);

mtx::events::MessageType
msg_type(const mtx::events::collections::TimelineEvents &event);
std::string
room_name(const mtx::events::collections::TimelineEvents &event);
std::string
room_topic(const mtx::events::collections::TimelineEvents &event);

std::string
body(const mtx::events::collections::TimelineEvents &event);

std::string
formatted_body(const mtx::events::collections::TimelineEvents &event);

QString
formattedBodyWithFallback(const mtx::events::collections::TimelineEvents &event);

std::optional<mtx::crypto::EncryptedFile>
file(const mtx::events::collections::TimelineEvents &event);

std::string
url(const mtx::events::collections::TimelineEvents &event);
std::string
thumbnail_url(const mtx::events::collections::TimelineEvents &event);
std::string
blurhash(const mtx::events::collections::TimelineEvents &event);
std::string
mimetype(const mtx::events::collections::TimelineEvents &event);
std::string
in_reply_to_event(const mtx::events::collections::TimelineEvents &event);
std::string
relates_to_event_id(const mtx::events::collections::TimelineEvents &event);
std::string
transaction_id(const mtx::events::collections::TimelineEvents &event);

int64_t
filesize(const mtx::events::collections::TimelineEvents &event);

uint64_t
media_height(const mtx::events::collections::TimelineEvents &event);

uint64_t
media_width(const mtx::events::collections::TimelineEvents &event);

nlohmann::json
serialize_event(const mtx::events::collections::TimelineEvents &event);
}
