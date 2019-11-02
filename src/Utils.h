#pragma once

#include <boost/variant.hpp>

#include "Cache.h"
#include "RoomInfoListItem.h"
#include "timeline/widgets/AudioItem.h"
#include "timeline/widgets/FileItem.h"
#include "timeline/widgets/ImageItem.h"
#include "timeline/widgets/VideoItem.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QPixmap>
#include <mtx/events/collections.hpp>
#include <mtx/events/common.hpp>

#include <qmath.h>

class QComboBox;

// Contains information about related events for
// outgoing messages
struct RelatedInfo
{
        using MsgType = mtx::events::MessageType;
        MsgType type;
        QString room;
        QString quoted_body;
        std::string related_event;
        QString quoted_user;
};

namespace utils {

using TimelineEvent = mtx::events::collections::TimelineEvents;

QString
replaceEmoji(const QString &body);

QString
localUser();

float
scaleFactor();

void
setScaleFactor(float factor);

//! Whether or not we should respond to key requests for the given room.
bool
respondsToKeyRequests(const QString &roomId);
bool
respondsToKeyRequests(const std::string &roomId);

void
setKeyRequestsPreference(QString roomId, bool value);

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

QString
event_body(const mtx::events::collections::TimelineEvents &event);

//! Match widgets/events with a description message.
template<class T>
QString
messageDescription(const QString &username = "",
                   const QString &body     = "",
                   const bool isLocal      = false)
{
        using Audio     = mtx::events::RoomEvent<mtx::events::msg::Audio>;
        using Emote     = mtx::events::RoomEvent<mtx::events::msg::Emote>;
        using File      = mtx::events::RoomEvent<mtx::events::msg::File>;
        using Image     = mtx::events::RoomEvent<mtx::events::msg::Image>;
        using Notice    = mtx::events::RoomEvent<mtx::events::msg::Notice>;
        using Sticker   = mtx::events::Sticker;
        using Text      = mtx::events::RoomEvent<mtx::events::msg::Text>;
        using Video     = mtx::events::RoomEvent<mtx::events::msg::Video>;
        using Encrypted = mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>;

        if (std::is_same<T, AudioItem>::value || std::is_same<T, Audio>::value) {
                if (isLocal)
                        return QCoreApplication::translate("message-description sent:",
                                                           "You sent an audio clip");
                else
                        return QCoreApplication::translate("message-description sent:",
                                                           "%1 sent an audio clip")
                          .arg(username);
        } else if (std::is_same<T, ImageItem>::value || std::is_same<T, Image>::value) {
                if (isLocal)
                        return QCoreApplication::translate("message-description sent:",
                                                           "You sent an image");
                else
                        return QCoreApplication::translate("message-description sent:",
                                                           "%1 sent an image")
                          .arg(username);
        } else if (std::is_same<T, FileItem>::value || std::is_same<T, File>::value) {
                if (isLocal)
                        return QCoreApplication::translate("message-description sent:",
                                                           "You sent a file");
                else
                        return QCoreApplication::translate("message-description sent:",
                                                           "%1 sent a file")
                          .arg(username);
        } else if (std::is_same<T, VideoItem>::value || std::is_same<T, Video>::value) {
                if (isLocal)
                        return QCoreApplication::translate("message-description sent:",
                                                           "You sent a video");
                else
                        return QCoreApplication::translate("message-description sent:",
                                                           "%1 sent a video")
                          .arg(username);
        } else if (std::is_same<T, StickerItem>::value || std::is_same<T, Sticker>::value) {
                if (isLocal)
                        return QCoreApplication::translate("message-description sent:",
                                                           "You sent a sticker");
                else
                        return QCoreApplication::translate("message-description sent:",
                                                           "%1 sent a sticker")
                          .arg(username);
        } else if (std::is_same<T, Notice>::value) {
                if (isLocal)
                        return QCoreApplication::translate("message-description sent:",
                                                           "You sent a notification");
                else
                        return QCoreApplication::translate("message-description sent:",
                                                           "%1 sent a notification")
                          .arg(username);
        } else if (std::is_same<T, Text>::value) {
                if (isLocal)
                        return QCoreApplication::translate("message-description sent:", "You: %1")
                          .arg(body);
                else
                        return QCoreApplication::translate("message-description sent:", "%1: %2")
                          .arg(username)
                          .arg(body);
        } else if (std::is_same<T, Emote>::value) {
                return QString("* %1 %2").arg(username).arg(body);
        } else if (std::is_same<T, Encrypted>::value) {
                if (isLocal)
                        return QCoreApplication::translate("message-description sent:",
                                                           "You sent an encrypted message");
                else
                        return QCoreApplication::translate("message-description sent:",
                                                           "%1 sent an encrypted message")
                          .arg(username);
        } else {
                return QCoreApplication::translate("utils", "Unknown Message Type");
        }
}

template<class T, class Event>
DescInfo
createDescriptionInfo(const Event &event, const QString &localUser, const QString &room_id)
{
        using Text  = mtx::events::RoomEvent<mtx::events::msg::Text>;
        using Emote = mtx::events::RoomEvent<mtx::events::msg::Emote>;

        const auto msg    = boost::get<T>(event);
        const auto sender = QString::fromStdString(msg.sender);

        const auto username = Cache::displayName(room_id, sender);
        const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);

        return DescInfo{QString::fromStdString(msg.event_id),
                        sender,
                        messageDescription<T>(username,
                                              QString::fromStdString(msg.content.body).trimmed(),
                                              sender == localUser),
                        utils::descriptiveTime(ts),
                        ts};
}

//! Scale down an image to fit to the given width & height limitations.
QPixmap
scaleDown(uint64_t maxWidth, uint64_t maxHeight, const QPixmap &source);

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

inline uint64_t
event_timestamp(const mtx::events::collections::TimelineEvents &event)
{
        return boost::apply_visitor([](auto msg) { return msg.origin_server_ts; }, event);
}

inline nlohmann::json
serialize_event(const mtx::events::collections::TimelineEvents &event)
{
        return boost::apply_visitor([](auto msg) { return json(msg); }, event);
}

inline mtx::events::EventType
event_type(const mtx::events::collections::TimelineEvents &event)
{
        return boost::apply_visitor([](auto msg) { return msg.type; }, event);
}

inline std::string
event_id(const mtx::events::collections::TimelineEvents &event)
{
        return boost::apply_visitor([](auto msg) { return msg.event_id; }, event);
}

inline QString
eventId(const mtx::events::collections::TimelineEvents &event)
{
        return QString::fromStdString(event_id(event));
}

inline QString
event_sender(const mtx::events::collections::TimelineEvents &event)
{
        return boost::apply_visitor([](auto msg) { return QString::fromStdString(msg.sender); },
                                    event);
}

template<class T>
QString
message_body(const mtx::events::collections::TimelineEvents &event)
{
        return QString::fromStdString(boost::get<T>(event).content.body);
}

//! Calculate the Levenshtein distance between two strings with character skipping.
int
levenshtein_distance(const std::string &s1, const std::string &s2);

QPixmap
scaleImageToPixmap(const QImage &img, int size);

//! Convert a Content Matrix URI to an HTTP link.
QString
mxcToHttp(const QUrl &url, const QString &server, int port);

//! Convert a ed25519 fingerprint into a human readable form
QString
humanReadableFingerprint(const std::string &ed25519);

QString
humanReadableFingerprint(const QString &ed25519);

//! Retrieve the message body taking into account the `formatted_body` field.
//! If the `format` of the message is not supported we fallback to `body`.
template<typename RoomMessageT>
QString
getMessageBody(const RoomMessageT &event)
{
        if (event.content.format.empty())
                return QString::fromStdString(event.content.body).toHtmlEscaped();

        if (event.content.format != common::FORMAT_MSG_TYPE)
                return QString::fromStdString(event.content.body).toHtmlEscaped();

        return QString::fromStdString(event.content.formatted_body);
}

//! Replace raw URLs in text with HTML link tags.
QString
linkifyMessage(const QString &body);

//! Convert the input markdown text to html.
QString
markdownToHtml(const QString &text);

//! Generate a Rich Reply quote message
QString
getFormattedQuoteBody(const RelatedInfo &related, const QString &html);

//! Get the body for the quote, depending on the event type.
QString
getQuoteBody(const RelatedInfo &related);

//! Retrieve the color of the links based on the current theme.
QString
linkColor();

//! Returns the hash code of the input QString
uint32_t
hashQString(const QString &input);

//! Generate a color (matching #RRGGBB) that has an acceptable contrast to background that is based
//! on the input string.
QString
generateContrastingHexColor(const QString &input, const QString &background);

//! Given two luminance values, compute the contrast ratio between them.
qreal
computeContrast(const qreal &one, const qreal &two);

//! Compute the luminance of a single color.  Based on https://stackoverflow.com/a/9733420
qreal
luminance(const QColor &col);

//! Center a widget in relation to another widget.
void
centerWidget(QWidget *widget, QWidget *parent);

void
restoreCombobox(QComboBox *combo, const QString &value);

struct SideBarSizes
{
        int small;
        int normal;
        int groups;
        int collapsePoint;
};

SideBarSizes
calculateSidebarSizes(const QFont &f);
}
