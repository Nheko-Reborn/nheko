// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <variant>

#include <CacheStructs.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QPixmap>
#include <QRegularExpression>
#include <mtx/events/collections.hpp>
#include <mtx/events/common.hpp>

#include <qmath.h>

struct DescInfo;

namespace cache {
// Forward declarations to prevent dependency on Cache.h, since this header is included often!
QString
displayName(const QString &room_id, const QString &user_id);
}

class QComboBox;

// Contains information about related events for
// outgoing messages
struct RelatedInfo
{
    using MsgType = mtx::events::MessageType;
    MsgType type;
    QString room;
    QString quoted_body, quoted_formatted_body;
    std::string related_event;
    QString quoted_user;
};

namespace utils {

using TimelineEvent = mtx::events::collections::TimelineEvents;

//! Helper function to remove reply fallback from body
std::string
stripReplyFromBody(const std::string &body);

//! Helper function to remove reply fallback from formatted body
std::string
stripReplyFromFormattedBody(const std::string &formatted_body);

RelatedInfo
stripReplyFallbacks(const TimelineEvent &event, std::string id, QString room_id_);

bool
codepointIsEmoji(uint code);

QString
replaceEmoji(const QString &body);

QString
localUser();

float
scaleFactor();

void
setScaleFactor(float factor);

//! Human friendly timestamp representation.
QString
descriptiveTime(const QDateTime &then);

//! Generate a message description from the event to be displayed
//! in the RoomList.
DescInfo
getMessageDescription(const TimelineEvent &event,
                      const QString &localUser,
                      const QString &displayName);

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
messageDescription(const QString &username = QString(),
                   const QString &body     = QString(),
                   const bool isLocal      = false)
{
    using Audio      = mtx::events::RoomEvent<mtx::events::msg::Audio>;
    using Emote      = mtx::events::RoomEvent<mtx::events::msg::Emote>;
    using File       = mtx::events::RoomEvent<mtx::events::msg::File>;
    using Image      = mtx::events::RoomEvent<mtx::events::msg::Image>;
    using Notice     = mtx::events::RoomEvent<mtx::events::msg::Notice>;
    using Sticker    = mtx::events::Sticker;
    using Text       = mtx::events::RoomEvent<mtx::events::msg::Text>;
    using Video      = mtx::events::RoomEvent<mtx::events::msg::Video>;
    using CallInvite = mtx::events::RoomEvent<mtx::events::voip::CallInvite>;
    using CallAnswer = mtx::events::RoomEvent<mtx::events::voip::CallAnswer>;
    using CallHangUp = mtx::events::RoomEvent<mtx::events::voip::CallHangUp>;
    using Encrypted  = mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>;

    if (std::is_same<T, Audio>::value) {
        if (isLocal)
            return QCoreApplication::translate("message-description sent:",
                                               "You sent an audio clip");
        else
            return QCoreApplication::translate("message-description sent:", "%1 sent an audio clip")
              .arg(username);
    } else if (std::is_same<T, Image>::value) {
        if (isLocal)
            return QCoreApplication::translate("message-description sent:", "You sent an image");
        else
            return QCoreApplication::translate("message-description sent:", "%1 sent an image")
              .arg(username);
    } else if (std::is_same<T, File>::value) {
        if (isLocal)
            return QCoreApplication::translate("message-description sent:", "You sent a file");
        else
            return QCoreApplication::translate("message-description sent:", "%1 sent a file")
              .arg(username);
    } else if (std::is_same<T, Video>::value) {
        if (isLocal)
            return QCoreApplication::translate("message-description sent:", "You sent a video");
        else
            return QCoreApplication::translate("message-description sent:", "%1 sent a video")
              .arg(username);
    } else if (std::is_same<T, Sticker>::value) {
        if (isLocal)
            return QCoreApplication::translate("message-description sent:", "You sent a sticker");
        else
            return QCoreApplication::translate("message-description sent:", "%1 sent a sticker")
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
            return QCoreApplication::translate("message-description sent:", "You: %1").arg(body);
        else
            return QCoreApplication::translate("message-description sent:", "%1: %2")
              .arg(username, body);
    } else if (std::is_same<T, Emote>::value) {
        return QStringLiteral("* %1 %2").arg(username, body);
    } else if (std::is_same<T, Encrypted>::value) {
        if (isLocal)
            return QCoreApplication::translate("message-description sent:",
                                               "You sent an encrypted message");
        else
            return QCoreApplication::translate("message-description sent:",
                                               "%1 sent an encrypted message")
              .arg(username);
    } else if (std::is_same<T, CallInvite>::value) {
        if (isLocal)
            return QCoreApplication::translate("message-description sent:", "You placed a call");
        else
            return QCoreApplication::translate("message-description sent:", "%1 placed a call")
              .arg(username);
    } else if (std::is_same<T, CallAnswer>::value) {
        if (isLocal)
            return QCoreApplication::translate("message-description sent:", "You answered a call");
        else
            return QCoreApplication::translate("message-description sent:", "%1 answered a call")
              .arg(username);
    } else if (std::is_same<T, CallHangUp>::value) {
        if (isLocal)
            return QCoreApplication::translate("message-description sent:", "You ended a call");
        else
            return QCoreApplication::translate("message-description sent:", "%1 ended a call")
              .arg(username);
    } else {
        return QCoreApplication::translate("utils", "Unknown Message Type");
    }
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

template<class T>
QString
message_body(const mtx::events::collections::TimelineEvents &event)
{
    return QString::fromStdString(std::get<T>(event).content.body);
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

    if (event.content.format != mtx::common::FORMAT_MSG_TYPE)
        return QString::fromStdString(event.content.body).toHtmlEscaped();

    return QString::fromStdString(event.content.formatted_body);
}

//! Replace raw URLs in text with HTML link tags.
QString
linkifyMessage(const QString &body);

//! Convert the input markdown text to html.
QString
markdownToHtml(const QString &text, bool rainbowify = false);

//! Escape every html tag, that was not whitelisted
QString
escapeBlacklistedHtml(const QString &data);

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
QColor
generateContrastingHexColor(const QString &input, const QColor &background);

//! Given two luminance values, compute the contrast ratio between them.
qreal
computeContrast(const qreal &one, const qreal &two);

//! Compute the luminance of a single color.  Based on https://stackoverflow.com/a/9733420
qreal
luminance(const QColor &col);

//! Center a widget in relation to another widget.
void
centerWidget(QWidget *widget, QWindow *parent);

void
restoreCombobox(QComboBox *combo, const QString &value);

//! Read image respecting exif orientation
QImage
readImageFromFile(const QString &filename);

//! Read image respecting exif orientation
QImage
readImage(const QByteArray &data);

bool
isReply(const mtx::events::collections::TimelineEvents &e);

void
removeDirectFromRoom(QString roomid);

void
markRoomAsDirect(QString roomid, std::vector<RoomMember> members);

std::vector<std::string>
roomVias(const std::string &roomid);
}
