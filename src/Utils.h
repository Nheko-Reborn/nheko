// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <CacheStructs.h>
#include <QDateTime>
#include <QPixmap>
#include <mtx/events.hpp>

namespace mtx::events::collections {
struct TimelineEvents;
struct StateEvents;
}

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
//! Helper function to remove reply fallback from body
std::string
stripReplyFromBody(const std::string &body);

//! Helper function to remove reply fallback from formatted body
std::string
stripReplyFromFormattedBody(const std::string &formatted_body);

RelatedInfo
stripReplyFallbacks(const mtx::events::collections::TimelineEvents &event,
                    std::string id,
                    QString room_id_);

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
getMessageDescription(const mtx::events::collections::TimelineEvents &event,
                      const QString &localUser,
                      const QString &displayName);

//! Get the first character of a string, taking into account that
//! surrogate pairs might be in use.
QString
firstChar(const QString &input);

//! Get a human readable file size with the appropriate units attached.
QString
humanReadableFileSize(uint64_t bytes);

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
markdownToHtml(const QString &text, bool rainbowify = false, bool noExtensions = false);

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

void
updateSpaceVias();

void
removeExpiredEvents();
}
