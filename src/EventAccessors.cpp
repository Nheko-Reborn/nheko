#include "EventAccessors.h"

#include <type_traits>

namespace {
struct nonesuch
{
        ~nonesuch()                = delete;
        nonesuch(nonesuch const &) = delete;
        void operator=(nonesuch const &) = delete;
};

namespace detail {
template<class Default, class AlwaysVoid, template<class...> class Op, class... Args>
struct detector
{
        using value_t = std::false_type;
        using type    = Default;
};

template<class Default, template<class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
{
        using value_t = std::true_type;
        using type    = Op<Args...>;
};

} // namespace detail

template<template<class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

struct EventMsgType
{
        template<class E>
        using msgtype_t = decltype(E::msgtype);
        template<class T>
        mtx::events::MessageType operator()(const mtx::events::Event<T> &e)
        {
                if constexpr (is_detected<msgtype_t, T>::value)
                        return mtx::events::getMessageType(e.content.msgtype);
                return mtx::events::MessageType::Unknown;
        }
};

struct EventRoomName
{
        template<class T>
        std::string operator()(const T &e)
        {
                if constexpr (std::is_same_v<mtx::events::StateEvent<mtx::events::state::Name>, T>)
                        return e.content.name;
                return "";
        }
};

struct EventRoomTopic
{
        template<class T>
        std::string operator()(const T &e)
        {
                if constexpr (std::is_same_v<mtx::events::StateEvent<mtx::events::state::Topic>, T>)
                        return e.content.topic;
                return "";
        }
};

struct EventBody
{
        template<class C>
        using body_t = decltype(C::body);
        template<class T>
        std::string operator()(const mtx::events::Event<T> &e)
        {
                if constexpr (is_detected<body_t, T>::value)
                        return e.content.body;
                return "";
        }
};

struct EventFormattedBody
{
        template<class C>
        using formatted_body_t = decltype(C::formatted_body);
        template<class T>
        std::string operator()(const mtx::events::RoomEvent<T> &e)
        {
                if constexpr (is_detected<formatted_body_t, T>::value)
                        return e.content.formatted_body;
                return "";
        }
};

struct EventFile
{
        template<class Content>
        using file_t = decltype(Content::file);
        template<class T>
        std::optional<mtx::crypto::EncryptedFile> operator()(const mtx::events::Event<T> &e)
        {
                if constexpr (is_detected<file_t, T>::value)
                        return e.content.file;
                return std::nullopt;
        }
};

struct EventUrl
{
        template<class Content>
        using url_t = decltype(Content::url);
        template<class T>
        std::string operator()(const mtx::events::Event<T> &e)
        {
                if constexpr (is_detected<url_t, T>::value) {
                        if (auto file = EventFile{}(e))
                                return file->url;
                        return e.content.url;
                }
                return "";
        }
};

struct EventThumbnailUrl
{
        template<class Content>
        using thumbnail_url_t = decltype(Content::info.thumbnail_url);
        template<class T>
        std::string operator()(const mtx::events::Event<T> &e)
        {
                if constexpr (is_detected<thumbnail_url_t, T>::value) {
                        return e.content.info.thumbnail_url;
                }
                return "";
        }
};

struct EventBlurhash
{
        template<class Content>
        using blurhash_t = decltype(Content::info.blurhash);
        template<class T>
        std::string operator()(const mtx::events::Event<T> &e)
        {
                if constexpr (is_detected<blurhash_t, T>::value) {
                        return e.content.info.blurhash;
                }
                return "";
        }
};

struct EventFilename
{
        template<class T>
        std::string operator()(const mtx::events::Event<T> &)
        {
                return "";
        }
        std::string operator()(const mtx::events::RoomEvent<mtx::events::msg::Audio> &e)
        {
                // body may be the original filename
                return e.content.body;
        }
        std::string operator()(const mtx::events::RoomEvent<mtx::events::msg::Video> &e)
        {
                // body may be the original filename
                return e.content.body;
        }
        std::string operator()(const mtx::events::RoomEvent<mtx::events::msg::Image> &e)
        {
                // body may be the original filename
                return e.content.body;
        }
        std::string operator()(const mtx::events::RoomEvent<mtx::events::msg::File> &e)
        {
                // body may be the original filename
                if (!e.content.filename.empty())
                        return e.content.filename;
                return e.content.body;
        }
};

struct EventMimeType
{
        template<class Content>
        using mimetype_t = decltype(Content::info.mimetype);
        template<class T>
        std::string operator()(const mtx::events::Event<T> &e)
        {
                if constexpr (is_detected<mimetype_t, T>::value) {
                        return e.content.info.mimetype;
                }
                return "";
        }
};

struct EventFilesize
{
        template<class Content>
        using filesize_t = decltype(Content::info.size);
        template<class T>
        int64_t operator()(const mtx::events::RoomEvent<T> &e)
        {
                if constexpr (is_detected<filesize_t, T>::value) {
                        return e.content.info.size;
                }
                return 0;
        }
};

struct EventInReplyTo
{
        template<class Content>
        using related_ev_id_t = decltype(Content::relates_to.in_reply_to.event_id);
        template<class T>
        std::string operator()(const mtx::events::Event<T> &e)
        {
                if constexpr (is_detected<related_ev_id_t, T>::value) {
                        return e.content.relates_to.in_reply_to.event_id;
                }
                return "";
        }
};

struct EventTransactionId
{
        template<class T>
        std::string operator()(const mtx::events::RoomEvent<T> &e)
        {
                return e.unsigned_data.transaction_id;
        }
        template<class T>
        std::string operator()(const mtx::events::Event<T> &e)
        {
                return e.unsigned_data.transaction_id;
        }
};

struct EventMediaHeight
{
        template<class Content>
        using h_t = decltype(Content::info.h);
        template<class T>
        uint64_t operator()(const mtx::events::Event<T> &e)
        {
                if constexpr (is_detected<h_t, T>::value) {
                        return e.content.info.h;
                }
                return -1;
        }
};

struct EventMediaWidth
{
        template<class Content>
        using w_t = decltype(Content::info.w);
        template<class T>
        uint64_t operator()(const mtx::events::Event<T> &e)
        {
                if constexpr (is_detected<w_t, T>::value) {
                        return e.content.info.w;
                }
                return -1;
        }
};

template<class T>
double
eventPropHeight(const mtx::events::RoomEvent<T> &e)
{
        auto w = eventWidth(e);
        if (w == 0)
                w = 1;

        double prop = eventHeight(e) / (double)w;

        return prop > 0 ? prop : 1.;
}
}

std::string
mtx::accessors::event_id(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit([](const auto e) { return e.event_id; }, event);
}
std::string
mtx::accessors::room_id(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit([](const auto e) { return e.room_id; }, event);
}

std::string
mtx::accessors::sender(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit([](const auto e) { return e.sender; }, event);
}

QDateTime
mtx::accessors::origin_server_ts(const mtx::events::collections::TimelineEvents &event)
{
        return QDateTime::fromMSecsSinceEpoch(
          std::visit([](const auto e) { return e.origin_server_ts; }, event));
}

std::string
mtx::accessors::filename(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventFilename{}, event);
}

mtx::events::MessageType
mtx::accessors::msg_type(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventMsgType{}, event);
}
std::string
mtx::accessors::room_name(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventRoomName{}, event);
}
std::string
mtx::accessors::room_topic(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventRoomTopic{}, event);
}

std::string
mtx::accessors::body(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventBody{}, event);
}

std::string
mtx::accessors::formatted_body(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventFormattedBody{}, event);
}

QString
mtx::accessors::formattedBodyWithFallback(const mtx::events::collections::TimelineEvents &event)
{
        auto formatted = formatted_body(event);
        if (!formatted.empty())
                return QString::fromStdString(formatted);
        else
                return QString::fromStdString(body(event)).toHtmlEscaped().replace("\n", "<br>");
}

std::optional<mtx::crypto::EncryptedFile>
mtx::accessors::file(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventFile{}, event);
}

std::string
mtx::accessors::url(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventUrl{}, event);
}
std::string
mtx::accessors::thumbnail_url(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventThumbnailUrl{}, event);
}
std::string
mtx::accessors::blurhash(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventBlurhash{}, event);
}
std::string
mtx::accessors::mimetype(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventMimeType{}, event);
}
std::string
mtx::accessors::in_reply_to_event(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventInReplyTo{}, event);
}

std::string
mtx::accessors::transaction_id(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventTransactionId{}, event);
}

int64_t
mtx::accessors::filesize(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventFilesize{}, event);
}

uint64_t
mtx::accessors::media_height(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventMediaHeight{}, event);
}

uint64_t
mtx::accessors::media_width(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(EventMediaWidth{}, event);
}

nlohmann::json
mtx::accessors::serialize_event(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit([](const auto &e) { return nlohmann::json(e); }, event);
}
